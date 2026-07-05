# Sygaldreye

WORK IN PROGRESS!

A dataflow authoring environment for empowering humans to understand, explore, and play with computation.

**Repository layout** (since the 2026-07 greenfield ratification):
`architecture/` — the book, the sole design document (start at its README) ·
`adr.md` — ratified decisions · `planning/` — vision, lexicon, status ·
`kanban/` — work items · `probe/` — the complete pre-greenfield
implementation, preserved as a deprecated design probe (reference and
salvage; build it from inside `probe/`). The feature bullets below describe
what the probe demonstrated and the greenfield rebuilds on the ratified
design.

- (Almost) everything is a node in a dataflow graph, including the graph editor, and also the graph itself
- Natively supports audio and graphics patching
- The graph transparently spans across heterogenous peer devices over the local area network
- Linux and Quest 3 native; WASM-based web support planned
- Linux peer supports cross-compiling and uploading `.so` plugins that the Quest peer can load at runtime, supporting continuous patching flow without having to restart the app or remove the headset on Quest; speech to text and text to speech built in, enabling communication with LLM coding assistants without removing the headset too
- Planned support for freezing sub-graphs to C++ source code targeting embedded devices and other targets

The goal is not to spend more time in virtual reality, but to enable the body to get involved in authoring programs that can be uploaded to physical appliances and digital musical instruments, so that the headset can be removed and the magic brought with you into regular reality.

Inspired by:

[Sygaldry](https://github.com/DocSunset/sygaldry)
[Pure Data](https://puredata.info/)
[Max/MSP](https://cycling74.com/products/max)
[Avendish](https://github.com/celtera/avendish)
[APL](https://en.wikipedia.org/wiki/APL_(programming_language))
[Forth](https://en.wikipedia.org/wiki/Forth_(programming_language))
[Doepfer](https://www.doepfer.de/home.htm)
[VCV Rack](https://vcvrack.com/)
[Pharo](https://pharo.org/)
[Uxn/Varvara](https://wiki.xxiivv.com/site/uxn.html)
[PICO-8](https://www.lexaloffle.com/pico-8.php)
[libmapper](https://libmapper.github.io/)
and many years thinking about how digital musical instruments work
