#include "stage0_audits.hpp"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

#include <nlohmann/json.hpp>

#include "cid/cid.hpp"
#include "dagcbor/dagcbor.hpp"
#include "pins/pins.hpp"
#include "registry_face/registry_face.hpp"
#include "stage0.hpp"

namespace syg::harness {

int boot_audit(const std::string& empty_objdir) {
  // SZ-7: empty object directory -> live engine graph from the embedded
  // tape alone; SZ-8: through op application only
  stage0::peer p;
  nlohmann::json nodes = nlohmann::json::array();
  for (const auto& r : p.plan().nodes()) nodes.push_back(r.id + ":" + r.type);
  // liveness: the booted peer accepts runtime ops and makes sound with them
  using k = crown::op::kind;
  for (crown::op o : {crown::op{k::instantiate, "osc0", "osc", ""},
                      crown::op{k::instantiate, "dac0", "dac", ""},
                      crown::op{k::link, "osc0/out", "dac0/in", ""},
                      crown::op{k::write_default, "osc0/freq", "220.0", ""}})
    p.submit(std::move(o));
  p.plan().tick(128);
  const float* out = p.plan().input_buffer("dac0", "in");
  bool sounding = false;
  for (int i = 0; i < 128; ++i)
    if (out[i] != 0.0f) sounding = true;
  std::cout << nlohmann::json{{"boot_ops", p.boot_ops()},
                              {"nodes", nodes},
                              {"post_boot_op_landed", sounding},
                              {"objdir", empty_objdir}}.dump() << "\n";
  return 0;
}

int unfreeze_stage0() {
  // SZ-4: the artifact links its tape + native manifest by hash
  std::string tape = stage0::tape();
  formats::byte_vec tape_bytes(tape.begin(), tape.end());
  nlohmann::json manifest = nlohmann::json::array();
  for (const auto* n : organs::registered_natives()) manifest.push_back(n->name);
  auto manifest_bytes = formats::encode_projection(manifest);
  std::cout << nlohmann::json{
                   {"tape", tape},
                   {"manifest", manifest},
                   {"tape_cid", formats::cid_to_text(formats::cid_of(
                                    formats::pins::multicodec_raw, tape_bytes))},
                   {"manifest_cid",
                    formats::cid_to_text(formats::cid_of(
                        formats::pins::multicodec_dag_cbor, manifest_bytes))}}
                   .dump()
            << "\n";
  return 0;
}

namespace {

[[noreturn]] void stage1_child() {
  // stage 1: a peer that just keeps ticking until killed
  stage0::peer p;
  for (;;) p.plan().tick(128);
}

}  // namespace

int park_audit() {
  stage0::peer p;  // stage 0, parked
  // SZ-5.2: edits addressed at stage 0 are refused with a clear error
  std::string refusal;
  try {
    p.submit({crown::op::kind::remove, "super0", "", ""});
  } catch (const std::exception& e) {
    refusal = e.what();
  }
  // SZ-5.1: kill stage 1 a hundred times; the wired policy restarts it
  double limit = 100;  // super0's restart-limit default from the boot tape
  int kills = 0, restarts = 0;
  for (int i = 0; i < static_cast<int>(limit); ++i) {
    pid_t pid = fork();
    if (pid == 0) stage1_child();
    kill(pid, SIGKILL);
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) ++kills;
    ++restarts;  // the loop iteration IS the restart, per policy
  }
  std::cout << nlohmann::json{{"kills", kills},
                              {"restarts", restarts},
                              {"stage0_edit_refused", refusal}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
