// Copyright (C) 2019 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "config.h"

#include "logic.hpp"
#include "xidrpcserver.hpp"

#include <xayagame/defaultmain.hpp>
#include <xayagame/game.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <jsonrpccpp/server/connectors/httpserver.h>

#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{

DEFINE_string (xaya_rpc_url, "",
               "URL at which Xaya Core's JSON-RPC interface is available");
DEFINE_int32 (game_rpc_port, 0,
              "the port at which xid's JSON-RPC server will be started"
              " (if non-zero)");

DEFINE_int32 (enable_pruning, -1,
              "if non-negative (including zero), old undo data will be pruned"
              " and only as many blocks as specified will be kept");

DEFINE_string (datadir, "",
               "base data directory for state data"
               " (will be extended by 'id' the chain)");

class XidInstanceFactory : public xaya::CustomisedInstanceFactory
{

private:

  /**
   * Reference to the XidGame instance.  This is needed to construct the
   * RPC server.
   */
  xid::XidGame& rules;

public:

  explicit XidInstanceFactory (xid::XidGame& r)
    : rules(r)
  {}

  std::unique_ptr<xaya::RpcServerInterface>
  BuildRpcServer (xaya::Game& game,
                  jsonrpc::AbstractServerConnector& conn) override
  {
    std::unique_ptr<xaya::RpcServerInterface> res;
    res.reset (new xaya::WrappedRpcServer<xid::XidRpcServer> (game, rules,
                                                              conn));
    return res;
  }

};

} // anonymous namespace

int
main (int argc, char** argv)
{
  google::InitGoogleLogging (argv[0]);

  gflags::SetUsageMessage ("Run Xaya ID daemon");
  gflags::SetVersionString (PACKAGE_VERSION);
  gflags::ParseCommandLineFlags (&argc, &argv, true);

  if (FLAGS_xaya_rpc_url.empty ())
    {
      std::cerr << "Error: --xaya_rpc_url must be set" << std::endl;
      return EXIT_FAILURE;
    }
  if (FLAGS_datadir.empty ())
    {
      std::cerr << "Error: --datadir must be specified" << std::endl;
      return EXIT_FAILURE;
    }

  xaya::GameDaemonConfiguration config;
  config.XayaRpcUrl = FLAGS_xaya_rpc_url;
  if (FLAGS_game_rpc_port != 0)
    {
      config.GameRpcServer = xaya::RpcServerType::HTTP;
      config.GameRpcPort = FLAGS_game_rpc_port;
    }
  config.EnablePruning = FLAGS_enable_pruning;
  config.DataDirectory = FLAGS_datadir;

  xid::XidGame rules;
  XidInstanceFactory instanceFact(rules);
  config.InstanceFactory = &instanceFact;

  return xaya::SQLiteMain (config, "id", rules);
}
