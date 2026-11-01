#!/usr/bin/env python3
"""STATUS 第13项的离线配置预演；无网络、串口、计时、设备状态或积分写入。"""
import argparse
import json
from pathlib import Path
import re
import time
import uuid

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DRAFT = ROOT / "assets/platform/ticko-agent/agent-draft.json"
STATES = {"IDLE", "PREPARING", "FOCUSING", "PAUSED", "RESTING", "READY_NEXT", "RECOVERY_REQUIRED"}


def load_draft(path=DEFAULT_DRAFT):
    return json.loads(Path(path).read_text(encoding="utf-8"))


def normalize(text):
    return re.sub(r"\s+", "", text).rstrip("。！？.!?")


def integer(value):
    return type(value) is int and value >= 0


def preview(text, context=None, *, command_id=None, draft=None):
    """只支持草案中的有限语料；不模拟平台LLM，不向任何设备发送命令。"""
    draft = draft if draft is not None else load_draft()
    if context is not None and not isinstance(context, dict):
        raise ValueError("模拟状态必须为JSON对象")
    context = context or {}
    result = {"simulation": True, "live_execution_enabled": False,
              "route": "help_fallback", "command": None}
    key = normalize(text)
    for phrase, reply in draft["clarifications"].items():
        if key == normalize(phrase):
            return dict(result, route="clarify", reply=reply)
    matches = [route for route in draft["routes"]
               if key in {normalize(example) for example in route["examples"]}]
    if len(matches) > 1:
        raise ValueError("草案语料对应多条路由")
    if not matches:
        reply = "这是一条知识问答，待平台检索对应知识库；没有发送设备指令。" if key in {
            normalize(example) for example in draft["help_examples"]
        } else "请明确要查询状态，还是开始、暂停或继续哪项活动；未发送设备指令。"
        return dict(result, reply=reply)
    route = matches[0]
    name = route["command"]
    spec = draft["commands"][name]
    params = dict(route["params"])
    if spec["kind"] == "business":
        if not (context.get("fresh") is True and integer(context.get("state_version"))
                and isinstance(context.get("boot_id"), str) and context["boot_id"]
                and context.get("state") in STATES):
            return dict(result, route="query_first", reply="需要先查询可信状态；尚未开始或改变计时。")
        if context["state"] not in spec["allowed_states"]:
            return dict(result, route="state_conflict", reply="当前状态不支持这项操作，需先核对设备状态。")
        if spec["requires_session"]:
            session_id = context.get("session_id")
            if not isinstance(session_id, str) or not session_id or len(session_id.encode()) > 64:
                return dict(result, route="query_first", reply="需要先取得当前会话编号；未操作设备。")
            params["session_id"] = session_id
        if name == "START_FOCUS":
            rules = context.get("mode_rules_ready")
            if not isinstance(rules, dict) or rules.get(params["mode"]) is not True:
                return dict(result, route="configuration_required", reply="该模式的规则还不完整，不能自行编造时长开始。")
    identifier = command_id if command_id is not None else "demo-" + uuid.uuid4().hex
    if not isinstance(identifier, str) or not identifier.startswith("demo-") or len(identifier.encode()) > 64:
        raise ValueError("离线命令ID必须以demo-开头且不超过64字节")
    command = {"command_id": identifier, "source": "agent_draft", "execution_source": "simulated",
               "name": name, "params": params,
               "clock_domain": "preview_host",
               "received_monotonic_ms": time.monotonic_ns() // 1_000_000}
    if spec["kind"] == "business":
        command.update(expected_state_version=context["state_version"], boot_id=context["boot_id"])
    if spec["kind"] == "property":
        command["property"] = {"siid": spec["binding"]["siid"], "piid": spec["binding"]["piid"], "value": params["value"]}
    return dict(result, route=route["group"], command=command,
                reply="仅生成模拟命令，尚未调用平台或真实设备。")


def describe_receipt(command, receipt):
    """检查模拟回执应怎样表述；真实回执必须另接经过验证的适配入口。"""
    result = {"confirmed": False, "simulation": True, "query_original": None}
    def answer(message, confirmed=False, query=False):
        return dict(result, confirmed=confirmed, reply="【模拟】" + message,
                    query_original=command["command_id"] if query else None)
    if receipt is None:
        return answer("暂未收到执行结果，先查询原命令，不换ID重发。", query=True)
    if not isinstance(receipt, dict):
        return answer("回执格式无效，先查询原命令。", query=True)
    if receipt.get("execution_source") != "simulated":
        return answer("离线工具不接收真实回执，不能确认设备执行。")
    if receipt.get("command_id") != command["command_id"]:
        return answer("回执与当前命令不匹配，不能确认执行。", query=True)
    if receipt.get("status") in {"accepted", "pending"}:
        return answer("请求已接收，仍在等待生效结果。")
    if receipt.get("status") == "rejected":
        return answer("设备拒绝了请求，尚未完成这项操作。")
    if receipt.get("status") != "applied":
        return answer("执行结果未知，先查询原命令。", query=True)
    if receipt.get("fresh") is not True:
        return answer("回执已过期或新鲜度未知，先查询当前状态。", query=True)
    if "boot_id" in command and receipt.get("boot_id") != command["boot_id"]:
        return answer("设备启动版本已变化，先查询恢复状态。", query=True)
    name, params = command["name"], command["params"]
    if name == "SET_VOLUME":
        observed = receipt.get("property", {})
        if not isinstance(observed, dict) or any(type(observed.get(k)) is not int or observed[k] != v
                                                for k, v in command["property"].items()):
            return answer("音量回报与目标属性或数值不一致。", query=True)
        return answer(f"设备回报音量为{observed['value']}；实际听音另行验证。", confirmed=True)
    state = receipt.get("state")
    version = receipt.get("state_version")
    if state not in STATES or not integer(version):
        return answer("缺少有效状态与版本，不能确认执行。", query=True)
    if name == "GET_STATUS":
        return answer(f"查询结果为{state}，版本{version}；未提供剩余时间时不推测。", confirmed=True)
    if version <= command["expected_state_version"]:
        return answer("状态版本未推进，先查询原命令。", query=True)
    if "session_id" in params and receipt.get("session_id") != params["session_id"]:
        return answer("会话编号不匹配，不能用旧会话回执确认。", query=True)
    expected = {"PAUSE_FOCUS": "PAUSED", "RESUME_FOCUS": "FOCUSING", "END_FOCUS": "IDLE", "END_USAGE": "IDLE"}
    if name == "START_FOCUS":
        if not isinstance(receipt.get("session_id"), str) or not receipt["session_id"]:
            return answer("缺少新会话编号，不能确认开始。", query=True)
        if state == "PREPARING":
            return answer("正在准备，还没确认开始计时。")
        if state == "FOCUSING":
            return answer("已开始计时。", confirmed=True)
    elif state == expected.get(name):
        message = {"PAUSE_FOCUS": "计时已暂停；姿态提醒按设备规则，未自行选择Q011。",
                   "RESUME_FOCUS": "已继续原会话计时。",
                   "END_FOCUS": "本轮已结束，不因此宣称完成整轮或发奖。",
                   "END_USAGE": "本次使用已结束，不撤销已有成长记录。"}[name]
        return answer(message, confirmed=True)
    return answer("实际状态与目标不一致，先查询原命令。", query=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("text")
    parser.add_argument("--context", type=Path, help="模拟状态JSON文件，不含真实凭证")
    args = parser.parse_args()
    context = json.loads(args.context.read_text(encoding="utf-8")) if args.context else None
    print(json.dumps(preview(args.text, context), ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
