#pragma once
// The authoring surface (AUT-3, ABI-1): port promises ride the field TYPE.

namespace syg::authoring {

// kind tags (the fiat starters; packages add their own)
struct audio { static constexpr const char* name = "audio"; };
struct scalar { static constexpr const char* name = "scalar"; };
struct fault { static constexpr const char* name = "fault"; };
struct bang { static constexpr const char* name = "bang"; };
struct span { static constexpr const char* name = "span"; };

// discipline tags: event / value / a clock name (ADR-020)
struct event { static constexpr const char* name = "event"; };
struct value { static constexpr const char* name = "value"; };
struct block { static constexpr const char* name = "block"; };
struct frame { static constexpr const char* name = "frame"; };

// A port field: the declaration is the type; the payload member is what a
// codec reads and a shell binds (scalar payloads at this rung).
template <class Kind, class Discipline>
struct in {
  float v = 0.0f;
  using kind = Kind;
  using discipline = Discipline;
  static constexpr const char* dir = "in";
};

template <class Kind, class Discipline>
struct out {
  float v = 0.0f;
  using kind = Kind;
  using discipline = Discipline;
  static constexpr const char* dir = "out";
};

}  // namespace syg::authoring
