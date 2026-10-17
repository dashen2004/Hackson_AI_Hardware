# Application Playback

This directory lives at `apps/main/lj_components/playback` and wraps the QUMI
`qm_player` and `qm_res_mgr` APIs used by
the resource playback guide.

The component provides:

- player creation around an existing shared `qm_audio` handle;
- URL and injected-file playback;
- resource list/category/detail lookup through the configured MQTT metadata channel;
- resource download through an application-owned data callback;
- pause, resume, stop, status, and teardown helpers.

`playback_init()` requires a decoder implementation and an optional local-file
source adapter. The component does not provide a codec backend or create the
audio handle. The caller owns both injected interfaces and must keep them alive
until `playback_deinit()` completes.

`apps/main/CMakeLists.txt` builds `lj_components/playback/playback.c` and exports
its include directory. Whether startup calls `playback_init()` depends on the
current initialization order in `main.c`.

Resource-manager list/detail calls are synchronous and must run from an
application worker, not an MQTT receive callback. `playback_play_*()` only
queues a request; use the player event log or the status API to observe the
actual lifecycle.

This app keeps `CONFIG_QM_PLAYER_CLOUD_SUPPORT` disabled. MQTT `player` command
routing remains application-owned, so this component does not register a second
`player` handler.
