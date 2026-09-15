#include "WinHttpFetch.hpp"

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
# define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

#include <atomic>
#include <string>

#include <QThread>

// the mingw-w64 8.1 headers predate these; the values are from the Windows SDK
#ifndef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
# define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
#endif
#ifndef WINHTTP_OPTION_DECOMPRESSION
# define WINHTTP_OPTION_DECOMPRESSION 118
#endif
#ifndef WINHTTP_DECOMPRESSION_FLAG_ALL
# define WINHTTP_DECOMPRESSION_FLAG_ALL 0x00000003
#endif

namespace
{
  // WinHTTP's own messages live in winhttp.dll, everything else in the system table
  QString error_text (DWORD code)
  {
    HMODULE const module = (code >= 12000 && code <= 12999) ? GetModuleHandleW (L"winhttp.dll") : nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    if (module) flags |= FORMAT_MESSAGE_FROM_HMODULE;
    wchar_t * buffer {nullptr};
    DWORD const length = FormatMessageW (flags, module, code, 0, reinterpret_cast<wchar_t *> (&buffer), 0, nullptr);
    QString text;
    if (length && buffer) text = QString::fromWCharArray (buffer, length).trimmed ();
    if (buffer) LocalFree (buffer);
    if (text.isEmpty ()) text = QString {"WinHTTP error %1"}.arg (code);
    return text;
  }

  LPCWSTR as_wide (QString const& s)
  {
    return reinterpret_cast<LPCWSTR> (s.utf16 ());
  }

  struct Handle
  {
    HINTERNET h {nullptr};
    ~Handle () { if (h) WinHttpCloseHandle (h); }
    operator HINTERNET () const { return h; }
  };
}

/* The fetch itself. Everything WinHTTP touches lives on this thread; the results are plain
   members the owner reads once QThread::finished has been delivered, so there is no shared
   mutable state and no lock beyond the cancellation flag. */
class WinHttpFetch::Worker final
  : public QThread
{
public:
  Worker (QString const& url, QString const& user_agent, qint64 max_body_size)
    : url_ {url}
    , user_agent_ {user_agent}
    , max_body_size_ {max_body_size}
  {
  }

  void cancel () { cancel_.store (true); }

  // valid once finished () has been emitted; error_ empty means fetched
  QByteArray body_;
  int status_ {0};
  QString error_;

protected:
  void run () override;

private:
  bool cancelled () const { return cancel_.load (); }

  QString const url_;
  QString const user_agent_;
  qint64 const max_body_size_;
  std::atomic<bool> cancel_ {false};
};

void WinHttpFetch::Worker::run ()
{
  auto fail_with_last_error = [this] (char const * call) {
      DWORD const code = GetLastError ();
      if (ERROR_WINHTTP_OPERATION_CANCELLED == code || cancelled ())
        {
          error_ = QString {"The download was cancelled."};
          return;
        }
      error_ = QString {"%1: %2"}.arg (QString::fromLatin1 (call)).arg (error_text (code));
    };

  Handle session, connection, request;

  // automatic proxy discovery needs Windows 8.1 or later; older systems take the static
  // configuration instead, which is what WinINet and Internet Options have always used
  session.h = WinHttpOpen (user_agent_.isEmpty () ? nullptr : as_wide (user_agent_),
                           WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session.h)
    {
      session.h = WinHttpOpen (user_agent_.isEmpty () ? nullptr : as_wide (user_agent_),
                               WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    }
  if (!session.h) { fail_with_last_error ("WinHttpOpen"); return; }

  // bounded so that a stalled server cannot hold up a cancel or the application's exit; the
  // receive timeout is per read, not for the whole transfer
  WinHttpSetTimeouts (session.h, 10000, 10000, 10000, 20000);

  // gzip where the platform offers it (Windows 8.1 and later); older ones just refuse the option
  DWORD decompression {WINHTTP_DECOMPRESSION_FLAG_ALL};
  WinHttpSetOption (session.h, WINHTTP_OPTION_DECOMPRESSION, &decompression, sizeof decompression);

  // deliberately no WINHTTP_OPTION_SECURE_PROTOCOLS: the system default already negotiates
  // TLS 1.2 and, on Windows 10 and later, TLS 1.3, and pinning a list here would switch 1.3 off
  // and freeze the policy in a release of JTDX rather than in Windows

  URL_COMPONENTS parts {};
  wchar_t host[256] {}, path[2048] {}, extra[2048] {};
  parts.dwStructSize = sizeof parts;
  parts.lpszHostName = host;    parts.dwHostNameLength = sizeof host / sizeof host[0];
  parts.lpszUrlPath = path;     parts.dwUrlPathLength = sizeof path / sizeof path[0];
  parts.lpszExtraInfo = extra;  parts.dwExtraInfoLength = sizeof extra / sizeof extra[0];
  if (!WinHttpCrackUrl (as_wide (url_), 0, 0, &parts)) { fail_with_last_error ("WinHttpCrackUrl"); return; }

  connection.h = WinHttpConnect (session.h, host, parts.nPort, 0);
  if (!connection.h) { fail_with_last_error ("WinHttpConnect"); return; }

  std::wstring target {path};
  target += extra;
  // WINHTTP_DEFAULT_ACCEPT_TYPES sends no Accept header at all, so name it as the Qt transport does
  LPCWSTR accept_types[] {L"*/*", nullptr};
  request.h = WinHttpOpenRequest (connection.h, L"GET", target.c_str (), nullptr, WINHTTP_NO_REFERER,
                                  accept_types,
                                  (INTERNET_SCHEME_HTTPS == parts.nScheme) ? WINHTTP_FLAG_SECURE : 0u);
  if (!request.h) { fail_with_last_error ("WinHttpOpenRequest"); return; }

  // redirects are followed by default, https to http is refused by default, and the default
  // limit is ten - the same three rules the Qt transport is given explicitly

  if (!WinHttpSendRequest (request.h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    { fail_with_last_error ("WinHttpSendRequest"); return; }
  if (!WinHttpReceiveResponse (request.h, nullptr))
    { fail_with_last_error ("WinHttpReceiveResponse"); return; }

  DWORD status {0};
  DWORD status_size {sizeof status};
  if (WinHttpQueryHeaders (request.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX))
    {
      status_ = int (status);
    }

  for (;;)
    {
      if (cancelled ()) { error_ = QString {"The download was cancelled."}; return; }
      DWORD available {0};
      if (!WinHttpQueryDataAvailable (request.h, &available))
        { fail_with_last_error ("WinHttpQueryDataAvailable"); return; }
      if (!available) break;                      // the response is complete
      QByteArray chunk (int (available), '\0');
      DWORD read {0};
      if (!WinHttpReadData (request.h, chunk.data (), available, &read))
        { fail_with_last_error ("WinHttpReadData"); return; }
      if (!read) break;
      chunk.truncate (int (read));
      body_ += chunk;
      if (body_.size () > max_body_size_) break;  // the caller reports it; reading on is waste
    }
}

WinHttpFetch::WinHttpFetch (QObject * parent)
  : QObject {parent}
{
}

WinHttpFetch::~WinHttpFetch ()
{
  if (!worker_) return;
  worker_->disconnect (this);                     // no signals into a dying object
  worker_->cancel ();
  if (worker_->wait (10000))
    {
      delete worker_;
    }
  else
    {
      // wedged inside a WinHTTP call past its own timeout: let the thread retire itself rather
      // than hold up the application, it owns everything it still touches
      connect (worker_, &QThread::finished, worker_, &QObject::deleteLater);
    }
  worker_ = nullptr;
}

void WinHttpFetch::start (QString const& url, QString const& user_agent, qint64 max_body_size)
{
  if (running ()) return;
  /* the previous fetch, already finished. Detached rather than deleted outright: its
     QThread::finished is a queued signal, so an invocation of worker_finished () can still be in
     flight for it, and that slot would otherwise read the results of whichever worker is current
     by then - emitting an empty fetched () and orphaning a running thread. */
  if (worker_) { worker_->disconnect (this); worker_->deleteLater (); worker_ = nullptr; }
  worker_ = new Worker {url, user_agent, max_body_size};
  connect (worker_, &QThread::finished, this, &WinHttpFetch::worker_finished);
  worker_->start ();
}

void WinHttpFetch::abort ()
{
  if (worker_) worker_->cancel ();
}

bool WinHttpFetch::running () const
{
  return worker_ && worker_->isRunning ();
}

void WinHttpFetch::worker_finished ()
{
  if (!worker_) return;
  Worker * const worker = worker_;
  auto const error = worker->error_;
  auto const body = worker->body_;
  auto const status = worker->status_;
  worker_ = nullptr;                              // running () is false before either signal
  worker->deleteLater ();
  if (error.isEmpty ()) Q_EMIT fetched (body, status);
  else Q_EMIT failed (error);
}
