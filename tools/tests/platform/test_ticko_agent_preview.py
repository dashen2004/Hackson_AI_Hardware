"""仅验证第13项离线草案契约，不访问平台、串口或真实状态。"""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/platform"))
from ticko_agent_preview import describe_receipt, load_draft, normalize, preview


class AgentDraftTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.draft = load_draft()
        cls.idle = json.loads((ROOT / "assets/platform/ticko-agent/mock-context.json").read_text())

    def contexts(self):
        focusing = dict(self.idle, state="FOCUSING", session_id="demo-session-1")
        return {
            "idle": self.idle, "ready_next": dict(self.idle, state="READY_NEXT"),
            "focusing": focusing, "paused": dict(focusing, state="PAUSED"),
            "resting": dict(focusing, state="RESTING"), "missing": {},
            "stale": dict(focusing, fresh=False), "no_session": dict(focusing, session_id=None),
            "unconfigured": dict(self.idle, mode_rules_ready={"reading": False}),
            "recovery": dict(self.idle, state="RECOVERY_REQUIRED"),
        }

    def test_forty_independent_utterances(self):
        cases = json.loads((ROOT / "assets/platform/ticko-agent/intent-cases.json").read_text())
        self.assertEqual(len({c["id"] for c in cases}), len(cases))
        for case in cases:
            with self.subTest(case=case["id"], text=case["text"]):
                result = preview(case["text"], self.contexts()[case["context"]])
                self.assertTrue(result["simulation"])
                self.assertFalse(result["live_execution_enabled"])
                self.assertEqual(result["route"], case["route"])
                command = result["command"]
                self.assertEqual(command["name"] if command else None, case["command"])
                if command:
                    for key, value in case.get("params", {}).items():
                        self.assertEqual(command["params"][key], value)

    def test_aliases_are_unique_and_point_to_defined_commands(self):
        used = set()
        for route in self.draft["routes"]:
            self.assertIn(route["command"], self.draft["commands"])
            for text in route["examples"]:
                self.assertNotIn(normalize(text), used)
                used.add(normalize(text))

    def test_prompt_fits_observed_role_field_and_knowledge_files_exist(self):
        prompt = (ROOT / "assets/platform/ticko-agent/role-prompt.txt").read_text()
        self.assertLessEqual(len(prompt), 2000)
        for name in self.draft["capabilities"]["knowledge_files"]:
            self.assertTrue((ROOT / name).is_file(), name)

    def test_business_bindings_remain_unassigned(self):
        for name, spec in self.draft["commands"].items():
            if name != "SET_VOLUME":
                self.assertIsNone(spec["binding"])
        self.assertFalse(self.draft["live_execution_enabled"])
        self.assertFalse(self.draft["platform_import_supported"])

    def test_volume_binding_matches_real_export_and_sdk_constant(self):
        model = json.loads((ROOT / "docs/开发/ticko-物模型-v3-20261031-脱敏.json").read_text())
        voice = next(s for s in model["services"] if s["serviceName"] == "voice")
        prop = next(p for p in voice["properties"] if p["piid"] == 1)
        binding = self.draft["commands"]["SET_VOLUME"]["binding"]
        self.assertEqual((voice["siid"], prop["piid"], prop["format"]),
                         (binding["siid"], binding["piid"], binding["format"]))
        header = (ROOT / "src/firmware/apps/main/lj_components/iot_control/ctrl/iot_control_volume.h").read_text()
        self.assertRegex(header, r"#define\s+IOT_CONTROL_VOLUME_SIID\s+2\b")

    def test_preview_does_not_modify_context_or_choose_unresolved_policies(self):
        context = self.contexts()["focusing"]
        before = copy.deepcopy(context)
        result = preview("暂停学习", context)
        self.assertEqual(context, before)
        self.assertNotIn("pause_posture_policy", result["command"]["params"])
        self.assertNotIn("duration", json.dumps(result["command"]))

    def test_proposed_retry_preserves_given_command_id(self):
        first = preview("开始阅读", self.idle, command_id="demo-original-1")["command"]
        second = preview("开始阅读", self.idle, command_id=first["command_id"])["command"]
        self.assertEqual(first["command_id"], second["command_id"])
        self.assertEqual(first["params"], second["params"])
        with self.assertRaises(ValueError):
            preview("开始阅读", self.idle, command_id="real-1")

    def test_invalid_state_version_never_produces_mutation(self):
        for value in [True, -1, "7", None]:
            with self.subTest(value=value):
                self.assertIsNone(preview("开始阅读", dict(self.idle, state_version=value))["command"])

    def test_invalid_context_or_rule_container_does_not_produce_command(self):
        with self.assertRaises(ValueError):
            preview("开始阅读", ["IDLE"])
        for value in [None, "ready", True, []]:
            with self.subTest(value=value):
                self.assertIsNone(preview("开始阅读", dict(self.idle, mode_rules_ready=value))["command"])

    def test_invalid_receipt_container_is_not_success(self):
        for value in [[], "applied", True]:
            with self.subTest(value=value):
                self.assertFalse(describe_receipt(self.pause_command(), value)["confirmed"])

    def pause_command(self):
        return preview("暂停学习", self.contexts()["focusing"], command_id="demo-pause-1")["command"]

    def applied_pause(self):
        return {"command_id": "demo-pause-1", "execution_source": "simulated", "status": "applied",
                "fresh": True, "state": "PAUSED", "state_version": 8,
                "session_id": "demo-session-1", "boot_id": "demo-boot-1"}

    def test_accepted_pending_and_rejected_are_not_success(self):
        for status in ["accepted", "pending", "rejected"]:
            with self.subTest(status=status):
                result = describe_receipt(self.pause_command(), dict(self.applied_pause(), status=status))
                self.assertFalse(result["confirmed"])
                self.assertNotIn("计时已暂停", result["reply"])

    def test_timeout_queries_original_id(self):
        result = describe_receipt(self.pause_command(), None)
        self.assertFalse(result["confirmed"])
        self.assertEqual(result["query_original"], "demo-pause-1")

    def test_mismatched_stale_or_incomplete_receipts_do_not_confirm(self):
        patches = [{"command_id": "demo-old"}, {"execution_source": "real"}, {"fresh": False},
                   {"boot_id": "demo-boot-2"}, {"state_version": 7}, {"state_version": True},
                   {"session_id": "demo-old-session"}, {"state": "FOCUSING"}, {"state": None},
                   {"status": "unrecognized"}]
        for patch in patches:
            with self.subTest(patch=patch):
                result = describe_receipt(self.pause_command(), dict(self.applied_pause(), **patch))
                self.assertFalse(result["confirmed"])

    def test_matching_applied_is_simulation_only(self):
        result = describe_receipt(self.pause_command(), self.applied_pause())
        self.assertTrue(result["confirmed"])
        self.assertTrue(result["simulation"])
        self.assertTrue(result["reply"].startswith("【模拟】"))

    def test_preparing_does_not_claim_timer_started(self):
        command = preview("开始阅读", self.idle, command_id="demo-start-1")["command"]
        receipt = dict(self.applied_pause(), command_id="demo-start-1", state="PREPARING")
        result = describe_receipt(command, receipt)
        self.assertFalse(result["confirmed"])
        self.assertNotIn("已开始计时", result["reply"])
        receipt["state"] = "FOCUSING"
        self.assertTrue(describe_receipt(command, receipt)["confirmed"])

    def test_property_report_requires_the_target_value_and_ids(self):
        command = preview("音量调到一半", command_id="demo-volume-1")["command"]
        receipt = {"command_id": "demo-volume-1", "execution_source": "simulated", "status": "applied",
                   "fresh": True, "property": {"siid": 2, "piid": 1, "value": 50}}
        self.assertTrue(describe_receipt(command, receipt)["confirmed"])
        for property_patch in [{"siid": 3}, {"piid": 3}, {"value": 0}, {"value": True}, {"value": "50"}]:
            with self.subTest(patch=property_patch):
                bad = dict(receipt, property=dict(receipt["property"], **property_patch))
                self.assertFalse(describe_receipt(command, bad)["confirmed"])


if __name__ == "__main__":
    unittest.main()
