#include "chunk.hpp"

#include <nlohmann/json.hpp>

#include "dagcbor/dagcbor.hpp"
#include "pins/pins.hpp"

namespace syg::formats {

byte_vec chunk_put(const byte_vec& blob,
                   const std::function<void(const byte_vec&, const byte_vec&)>& put) {
  if (blob.size() <= pins::chunk_bytes) {
    auto cid = cid_of(pins::multicodec_raw, blob);
    put(cid, blob);
    return cid;
  }
  nlohmann::json index = nlohmann::json::array();
  for (std::size_t off = 0; off < blob.size(); off += pins::chunk_bytes) {
    byte_vec chunk(blob.begin() + static_cast<long>(off),
                   blob.begin() + static_cast<long>(
                                      std::min(off + pins::chunk_bytes, blob.size())));
    auto cid = cid_of(pins::multicodec_raw, chunk);
    put(cid, chunk);
    index.push_back({{"/", cid_to_text(cid)}});
  }
  auto index_bytes = encode_projection(index);
  auto root = cid_of(pins::multicodec_dag_cbor, index_bytes);
  put(root, index_bytes);
  return root;
}

}  // namespace syg::formats
