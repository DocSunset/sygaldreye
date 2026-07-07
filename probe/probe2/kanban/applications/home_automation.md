# Home automation (brainstorm 2026-06-23)

Application note, not yet a work item.

Use the graph to control and monitor a smart home — devices, sensors,
and routines as wirable nodes. Spatial/VR control surface (reach out and
touch a light), voice control via the existing speech loop, and sensor
streams feeding the patch like any other edge.

Open questions: protocol/bridge (Home Assistant API? MQTT? Matter?),
where the bridge runs (host worker vs device), and how device state maps
to ports/edges (events vs continuous values).
