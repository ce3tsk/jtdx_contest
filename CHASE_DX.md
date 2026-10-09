# Chase DX (wait for TX)

A DXpedition tool: name the expedition station, and JTDX waits. The moment that station is
decoded - whether it is running CQ or answering somebody else - JTDX arms TX and calls it.
If it has not been heard for N of our calls, JTDX stops calling and goes back to waiting, and
resumes the moment it is heard again. The loop runs until the contact is logged. It works with
ordinary DXpeditions as well as Fox/hound and SuperFox; nothing about the DXpedition's own mode
is required.

## States

| state | TX | what is happening |
|---|---|---|
| `ChaseOff` | - | not armed; the Halt Tx button looks normal |
| `ChaseWait` | off | armed, no counter running, waits indefinitely for the target |
| `ChaseCalling` | on | calling the target, counting the calls it has not answered |

## Getting in and out

* **Right click on Halt Tx** arms and disarms. While armed the button reads **`chaseDX`** on a
  blue field of its own (dark mode safe) and its tooltip says so; afterwards it is restored
  exactly as the `.ui` made it. A plain left click is still Halt Tx.
* `DXpedition -> Chase DX (wait for TX)` is the same switch and is kept in step with the button.
* `DXpedition -> Chase: periods without the DX...` sets the budget (default **6**, 1-99). Only
  this is persisted, as `ChasePeriods`.
* A chase **always starts switched off** - it is never restored from the settings file.

## Arming is validated

Both routes require, as the operator specified:

1. a call in the **DX Call** box, and
2. **TX1-TX5 already generated for that call with Gen Msgs**.

If either is missing the chase does not arm, the button is not coloured, and the status bar
says which half is missing. (tx1..tx4 are line edits, tx5 is the combo of 73/report
alternatives.)

## Behaviour

* **Trigger: the target must be the sender of the decoded message.** In FT8 the sender is the
  second call (`CQ <me> <grid>`, `<them> <me> <grid|report>`), i.e. the second call of
  `DecodedText::deCallAndGrid()`. Being called by somebody else does not trigger. The hook sits
  in the post-decode block, ahead of the `prevent_spotting_false()` branch, so every decode
  passes through it.
* **The budget counts our calls, not decode slots.** `<DecodeFinished>` fires in our TX slot and
  in the DX's, so one increment per callback would spend a six-call budget in three calls. The
  miss counter moves once per period in which one of our own transmissions actually started
  (`m_txPeriod`).
* **A running chase keeps calling.** The application's own stops must not end something the
  operator asked to keep running: the minutes-long TX watchdog (`WD:` in the status bar) is
  reset every period and Enable Tx is re-armed if anything dropped it, and the singleshot /
  hound "counter triggered" stop is skipped while a chase is calling. **To stop a chase, switch
  it off** (right click or menu) - not by waiting for a timer.
* **Completion**: a logged contact ends the chase (`on_logQSOButton_clicked`, which the auto-log
  timer also uses) and unchecks it. The *give-up* exits are deliberately not treated as
  completion - hooking `endOfQsoStopTx()` (which also carries "RR73/73 never received", the
  answer counters and "not owner of the frequency") ended chases that should have kept running.
* **Not persisted, no auto-QSY, no widening of the search** (the operator's choices).

## Log

State changes go to `YYYYMM_ALL.TXT` unconditionally, prefixed `Chase:`:

```
Chase: armed, waiting for UA9QFF, 6 periods
Chase: UA9QFF heard - calling
Chase: 1/6 calls without UA9QFF
Chase: UA9QFF heard again - counter reset
Chase: 6/6 calls without UA9QFF
Chase: back to waiting for UA9QFF
Chase: QSO complete - chase off (was UA9QFF)
```

Per-call counter detail and the button transitions only appear with `JTDX_CHASE_TRACE=1`.

## Files

`mainwindow.cpp` / `mainwindow.h` hold the state machine, the menu entries, the Halt Tx button
handling and the validation; `sequencer_hooks.cpp` carries a note on why completion is *not*
hooked there. A chase status label sits next to the TX watchdog label in the status bar.

## Test hooks

```
JTDX_CHASE_TRACE=1        per-call counter detail in ALL.TXT
JTDX_CHASE_DRYRUN=1       run the state machine without ever keying the rig
JTDX_CHASE_ARM=<call>     arm at start-up (presses Gen Msgs first) to drive an unattended instance
```

## What was verified, and what needs the operator

Verified here: arming through `JTDX_CHASE_ARM` (which exercises the DX Call + Gen Msgs
validation), the button transition to `chaseDX`, `waiting -> heard -> calling` on live signals
in dry run, and file-mode decoding at 1/2/15 threads after the MSYS2 fix in `BUILD_MSYS2.md`.

Needs a real transmitter: that a chase **keeps** calling across the watchdog and the answer
counters, that the budget expires after six *calls* without the target, and that a completed
contact switches the chase off. The "give up" paths that used to kill a chase were reproduced
and fixed from the operator's report; the log lines above are the way to confirm the behaviour.

---
---

# Chase DX（等待发射）

远征台追逐工具：输入远征台呼号，JTDX 进入守候；**一旦解出该台**（无论它在 CQ 还是在应答别人），
立即转入发射并呼叫它。若连续 N 次我方呼叫都没再听到它，则停止呼叫、回到守候，下次听到再恢复呼叫；
如此循环，直到通联记录完成为止。普通远征台、Fox/hound、SuperFox 都适用，不要求对方开启任何特殊模式。

## 状态

| 状态 | 发射 | 含义 |
|---|---|---|
| `ChaseOff` | — | 未开启；HaltTx 按钮为常态外观 |
| `ChaseWait` | 关 | 已开启，不计时，无限期守候目标 |
| `ChaseCalling` | 开 | 正在呼叫目标，统计它未回应的呼叫次数 |

## 如何开关

* **右键 HaltTx** 开启/关闭。开启期间按钮文字变为 **`chaseDX`**，底色为专属蓝色（适配暗色主题），
  tooltip 同步提示；关闭后**逐字还原**为 `.ui` 原本的文字/样式/tooltip。左键仍是 HaltTx（停发射）。
* `DXpedition → Chase DX (wait for TX)` 菜单与按钮是同一个开关，两者状态始终一致。
* `DXpedition → Chase: periods without the DX...` 设置配额（默认 **6**，范围 1–99），以
  `ChasePeriods` 持久化。
* **chase 启动时一律是关闭的**，不会从配置文件恢复。

## 开启前的校验

两条入口（右键 / 菜单）都要满足：

1. **DX Call** 框里有目标呼号；
2. **TX1–TX5 已由 GenMsgs 生成为该呼号的消息**。

任一不满足则不开启，按钮不变色，状态栏提示缺什么（tx1..tx4 是文本框，tx5 是 73/报告选项下拉框）。

## 行为

* **触发条件：目标必须是该条解码消息的发射方**。FT8 里发射方排第二个呼号
  （`CQ <我> <网格>`、`<对方> <我> <网格|报告>`），即 `DecodedText::deCallAndGrid()` 的第二个呼号。
  别人呼叫它不算触发。钩子位于每条解码必经的 postDecode 块内、`prevent_spotting_false()` 分支之前。
* **配额按"我方呼叫次数"计，不按时隙数**。`<DecodeFinished>` 在我方发射时隙与对方时隙都会触发，
  若每个回调都加一，6 次的配额会在 3 次呼叫内用光。计数器只在**我方确实发起过发射**的周期递增
  （依据 `m_txPeriod`）。
* **呼叫期间必须一直叫下去**。程序自身的停机不得中断操作员要求持续进行的 chase：每分钟级的
  TX 看门狗（状态栏 `WD:`）每周期被复位，Enable Tx 若被某处关掉会自动重新打开；singleshot /
  hound 的"counter triggered"停机在 chase 期间被跳过。**要停 chase 就关掉它**（右键或菜单），
  而不是等定时器。
* **通联完成**：QSO 被记录后结束 chase 并自动取消勾选（走 `on_logQSOButton_clicked`，自动记录同样经过它）。
  **"放弃"类退出不算完成**——曾把收尾挂在 `endOfQsoStopTx()`（它同时承载"RR73/73 未收到"、应答计数器、
  "失去频率权"等放弃路径），结果把本该继续的 chase 取消了。
* **不持久化、不自动 QSY、不扩大搜索范围**（按操作员选择）。

## 日志

状态变化无条件写入 `YYYYMM_ALL.TXT`，前缀 `Chase:`：

```
Chase: armed, waiting for UA9QFF, 6 periods
Chase: UA9QFF heard - calling
Chase: 1/6 calls without UA9QFF
Chase: UA9QFF heard again - counter reset
Chase: 6/6 calls without UA9QFF
Chase: back to waiting for UA9QFF
Chase: QSO complete - chase off (was UA9QFF)
```

每次呼叫的计数细节与按钮切换需 `JTDX_CHASE_TRACE=1`。

## 涉及文件

状态机、菜单项、HaltTx 按钮处理与校验在 `mainwindow.cpp` / `mainwindow.h`；`sequencer_hooks.cpp`
里有一条注释说明**为什么通联完成不挂在那里**。状态栏 TX 看门狗标签旁新增 chase 状态标签。

## 测试钩子

```
JTDX_CHASE_TRACE=1        每次呼叫的计数细节写入 ALL.TXT
JTDX_CHASE_DRYRUN=1       只跑状态机，绝不发射
JTDX_CHASE_ARM=<呼号>     启动即开启（会先点 GenMsgs），用于驱动无人值守的测试实例
```

## 已验证 / 待操作员实测

已在此验证：通过 `JTDX_CHASE_ARM` 开启（顺带验证了 DX Call + GenMsgs 校验）、按钮切到 `chaseDX`、
实时信号下 `waiting → heard → calling`（dry-run）、以及 MSYS2 修复后 1/2/15 线程文件模式解码
（见 `BUILD_MSYS2.md`）。

需要真发射才能验证：chase 在看门狗与应答计数器下**是否持续呼叫**、配额是否确实在
**6 次呼叫**后到期、以及通联完成后是否自动关闭。曾经掐断 chase 的"放弃"路径已按操作员反馈定位并修复；
确认行为请对照上面的日志行。
