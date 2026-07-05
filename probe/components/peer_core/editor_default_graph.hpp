// Copyright 2026 Travis West
#pragma once

// The node-based editor that both shells boot into by default. Mirrors
// assets/graphs/editor.json (graph_source → lifted card subgraph → draw +
// gesture nodes). Embedded so the device shell (which packages assets at
// runtime, after init) boots the editor with no filesystem dependency.
//
// kEditorCardSubgraph mirrors assets/graphs/card.json; both shells register
// it as the `card` subgraph type before parsing kEditorGraph.
namespace editor_default_graph {

inline constexpr const char* kEditorCardSubgraph = R"({
  "inlets":[
    {"name":"id","node":"geo","port":"id"},
    {"name":"position","node":"geo","port":"position"}
  ],
  "outlets":[
    {"name":"mesh","node":"geo","port":"mesh"}
  ],
  "lift_key":"id",
  "nodes":[
    {"id":"geo","type":"card_mesh","params":{}}
  ],
  "edges":[]
})";

inline constexpr const char* kEditorGraph = R"({
  "nodes":[
    {"id":"camera","type":"fly_camera","params":{}},
    {"id":"head","type":"render_head","params":{}},

    {"id":"hand_l","type":"hand","params":{"x":-0.25}},
    {"id":"hand_r","type":"hand","params":{"x":0.25}},

    {"id":"gsrc","type":"graph_source","params":{}},
    {"id":"card","type":"card","params":{}},
    {"id":"paint","type":"color_mesh","params":{"r":0.13,"g":0.17,"b":0.24}},
    {"id":"cards","type":"forest_draw","params":{}},

    {"id":"widgets","type":"card_widgets_mesh","params":{}},
    {"id":"wpaint","type":"vertex_color_mesh","params":{}},
    {"id":"wdraw","type":"draw","params":{}},

    {"id":"labels","type":"card_labels_mesh","params":{}},
    {"id":"ldraw","type":"draw","params":{}},

    {"id":"ewires","type":"editor_wires","params":{}},
    {"id":"wires","type":"wire_mesh","params":{}},
    {"id":"wiredraw","type":"draw","params":{}},

    {"id":"hover","type":"handle_picker","params":{}},
    {"id":"hovertext","type":"text_label","params":{"scale":0.012}},
    {"id":"hoverdraw","type":"draw","params":{}},

    {"id":"drag","type":"wire_drag","params":{}},
    {"id":"slide","type":"slider_drag","params":{}},
    {"id":"dwell","type":"dwell_delete","params":{}},
    {"id":"undo","type":"undo_node","params":{}},
    {"id":"palette","type":"palette","params":{}},

    {"id":"palmesh","type":"palette_mesh","params":{}},
    {"id":"paneldraw","type":"draw","params":{}},
    {"id":"palabeldraw","type":"draw","params":{}}
  ],
  "edges":[
    {"from":"gsrc.keys","to":"card.id"},
    {"from":"gsrc.positions","to":"card.position"},
    {"from":"card.mesh","to":"cards.meshes"},
    {"from":"paint.surface","to":"cards.surface"},

    {"from":"widgets.mesh","to":"wpaint.geometry"},
    {"from":"wpaint.mesh","to":"wdraw.mesh"},
    {"from":"wpaint.surface","to":"wdraw.surface"},

    {"from":"labels.mesh","to":"ldraw.mesh"},
    {"from":"labels.surface","to":"ldraw.surface"},

    {"from":"ewires.wires","to":"wires.wires"},
    {"from":"wires.mesh","to":"wiredraw.mesh"},
    {"from":"wires.surface","to":"wiredraw.surface"},

    {"from":"hand_r.pos","to":"hover.pos"},
    {"from":"hand_r.rot","to":"hover.rot"},
    {"from":"hover.label","to":"hovertext.text"},
    {"from":"hovertext.mesh","to":"hoverdraw.mesh"},
    {"from":"hovertext.surface","to":"hoverdraw.surface"},

    {"from":"hand_r.pos","to":"drag.pos"},
    {"from":"hand_r.rot","to":"drag.rot"},
    {"from":"hand_r.grip","to":"drag.grip"},

    {"from":"hand_r.pos","to":"slide.pos"},
    {"from":"hand_r.rot","to":"slide.rot"},
    {"from":"hand_r.trigger","to":"slide.trigger"},

    {"from":"hand_r.pos","to":"dwell.pos"},
    {"from":"hand_r.rot","to":"dwell.rot"},
    {"from":"hand_r.grip","to":"dwell.grip"},

    {"from":"hand_l.thumb_x","to":"undo.thumb_x"},

    {"from":"hand_r.pos","to":"palette.pos"},
    {"from":"hand_r.rot","to":"palette.rot"},
    {"from":"hand_r.trigger","to":"palette.trigger"},

    {"from":"palmesh.panel","to":"paneldraw.mesh"},
    {"from":"palmesh.panel_surface","to":"paneldraw.surface"},
    {"from":"palmesh.labels","to":"palabeldraw.mesh"},
    {"from":"palmesh.labels_surface","to":"palabeldraw.surface"}
  ]
})";

}  // namespace editor_default_graph
