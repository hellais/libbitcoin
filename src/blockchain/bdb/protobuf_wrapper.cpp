#include "protobuf_wrapper.hpp"

#include <bitcoin/transaction.hpp>

namespace libbitcoin {

struct bdb_proto_shutdown
{
    ~bdb_proto_shutdown()
    {
        google::protobuf::ShutdownProtobufLibrary();
    }
} proto_shutdown_inst;

proto::Block block_header_to_proto(uint32_t depth,
    const block_type serial_block)
{
    proto::Block proto_block;
    proto_block.set_depth(depth);
    proto_block.set_version(serial_block.version);
    proto_block.set_previous_block_hash(
        serial_block.previous_block_hash.data(),
        serial_block.previous_block_hash.size());
    proto_block.set_merkle(
        serial_block.merkle.data(), serial_block.merkle.size());
    proto_block.set_timestamp(serial_block.timestamp);
    proto_block.set_bits(serial_block.bits);
    proto_block.set_nonce(serial_block.nonce);
    return proto_block;
}

proto::Transaction transaction_to_proto(
    const transaction_type& block_tx)
{
    proto::Transaction proto_tx;
    proto_tx.set_version(block_tx.version);
    proto_tx.set_locktime(block_tx.locktime);
    for (const transaction_input_type& block_input: block_tx.inputs)
    {
        proto::Transaction::Input& proto_input = *proto_tx.add_inputs();
        proto_input.set_previous_output_hash(
            block_input.previous_output.hash.data(),
            block_input.previous_output.hash.size());
        proto_input.set_previous_output_index(
            block_input.previous_output.index);
        data_chunk raw_script = save_script(block_input.input_script);
        proto_input.set_script(&raw_script[0], raw_script.size());
        proto_input.set_sequence(block_input.sequence);
    }
    for (const transaction_output_type& block_output: block_tx.outputs)
    {
        proto::Transaction::Output& proto_output = *proto_tx.add_outputs();
        proto_output.set_value(block_output.value);
        data_chunk raw_script = save_script(block_output.output_script);
        proto_output.set_script(&raw_script[0], raw_script.size());
    }
    return proto_tx;
}

block_type proto_to_block_header(const proto::Block& proto_block)
{
    block_type result_block;
    result_block.version = proto_block.version();
    const std::string& previous_block_hash = proto_block.previous_block_hash();
    std::copy(previous_block_hash.begin(), previous_block_hash.end(),
        result_block.previous_block_hash.begin());
    const std::string& merkle = proto_block.merkle();
    std::copy(merkle.begin(), merkle.end(), result_block.merkle.begin());
    result_block.timestamp = proto_block.timestamp();
    result_block.bits = proto_block.bits();
    result_block.nonce = proto_block.nonce();
    return result_block;
}

// Ugly boilerplate for ugly transformation
template <typename InOut>
data_chunk read_raw_script(const InOut& inout)
{
    const std::string& script_str = inout.script();
    return data_chunk(script_str.begin(), script_str.end());
}

transaction_type proto_to_transaction(
    const proto::Transaction& proto_tx)
{
    transaction_type result_tx;
    result_tx.version = proto_tx.version();
    result_tx.locktime = proto_tx.locktime();
    for (size_t i = 0; i < proto_tx.inputs_size(); ++i)
    {
        const proto::Transaction::Input& proto_input = proto_tx.inputs(i);
        transaction_input_type tx_input;
        const std::string& prev_out_hash = proto_input.previous_output_hash();
        std::copy(prev_out_hash.begin(), prev_out_hash.end(),
            tx_input.previous_output.hash.begin());
        tx_input.previous_output.index = proto_input.previous_output_index();
        const data_chunk& raw_script = read_raw_script(proto_input);
        if (previous_output_is_null(tx_input.previous_output))
            tx_input.input_script = coinbase_script(raw_script);
        else
            tx_input.input_script = parse_script(raw_script);
        tx_input.sequence = proto_input.sequence();
        result_tx.inputs.push_back(tx_input);
    }
    for (size_t i = 0; i < proto_tx.outputs_size(); ++i)
    {
        const proto::Transaction::Output& proto_output = proto_tx.outputs(i);
        transaction_output_type tx_output;
        tx_output.value = proto_output.value();
        tx_output.output_script = parse_script(read_raw_script(proto_output));
        result_tx.outputs.push_back(tx_output);
    }
    return result_tx;
}

} // namespace libbitcoin

