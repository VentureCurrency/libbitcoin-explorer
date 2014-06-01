/*
 * Copyright (c) 2011-2014 sx developers (see AUTHORS)
 *
 * This file is part of sx.
 *
 * sx is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/algorithm/string.hpp>
#include <bitcoin/bitcoin.hpp>
#include <obelisk/obelisk.hpp>
#include "config.hpp"
#include <sx/utility/console.hpp>

using namespace bc;

bool stopped = false;

void block_header_fetched(const std::error_code& ec,
    const block_header_type& blk_header)
{
    if (ec)
    {
        std::cerr << "fetch-block-header: " << ec.message() << std::endl;
        stopped = true;
        return;
    }
    data_chunk raw_blk_header(satoshi_raw_size(blk_header));
    satoshi_save(blk_header, raw_blk_header.begin());
    std::cout << raw_blk_header << std::endl;
    stopped = true;
}

bool invoke(const int argc, const char* argv[])
{
    std::string index_str;
    if (argc == 2)
        index_str = argv[1];
    else
        index_str = read_stdin();
    config_map_type config;
    get_config(config);
    threadpool pool(1);
    obelisk::fullnode_interface fullnode(pool, config["service"],
        config["client-certificate"], config["server-public-key"]);
    // Try first to interpret index as hash, if that fails then
    // interpret the index as a height instead.
    hash_digest blk_hash = decode_hash(index_str);
    if (blk_hash == null_hash)
    {
        size_t height = 0;
        boost::algorithm::trim(index_str);
        try
        {
            height = boost::lexical_cast<size_t>(index_str);
        }
        catch (const boost::bad_lexical_cast&)
        {
            std::cerr << "fetch-block-header: Bad index provided." << std::endl;
            return -1;
        }
        fullnode.blockchain.fetch_block_header(height, block_header_fetched);
    }
    else
    {
        fullnode.blockchain.fetch_block_header(blk_hash, block_header_fetched);
    }
    while (!stopped)
    {
        fullnode.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    pool.stop();
    pool.join();
    return 0;
}
