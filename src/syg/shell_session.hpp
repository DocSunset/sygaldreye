#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// syg shell (PKG-4.4): the ordinary peer with a render executor. Under test,
// {graph, size:[w,h], offscreen:true, script:[...]}; script steps:
//   {pointer:{x,y,buttons}}  feed the pointer SOURCE node (N4 — no side door)
//   {settle:true}            pump blocks so gesture ops reach the arbiter
//   {frame:true}             append one RGBA8 frame to stdout
//   {doc:true}               append the persisted doc as JSON after the frames
int shell_session(const nlohmann::json& in);

}  // namespace syg::harness
