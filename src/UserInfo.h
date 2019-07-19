/*
 The MIT License (MIT)

 Copyright (c) [2016] [BTC.COM]

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */
#pragma once

#include <memory>
#include <algorithm>
#include <shared_mutex>

#include "utilities_js.hpp"

class StratumServer;

///////////////////////////////////// UserInfo /////////////////////////////////
// 1. update userName->userId by interval
// 2. insert worker name to db
class UserInfo {
  struct WorkerName {
    int32_t userId_;
    int64_t workerId_;
    char workerName_[21];
    char minerAgent_[31];

    WorkerName()
      : userId_(0)
      , workerId_(0) {
      memset(workerName_, 0, sizeof(workerName_));
      memset(minerAgent_, 0, sizeof(minerAgent_));
    }
  };

  struct ChainVars {
    string apiUrl_;

    std::unique_ptr<std::shared_timed_mutex> nameIdlock_;
    // username -> userId
    std::unordered_map<string, int32_t> nameIds_;
    int32_t lastMaxUserId_;
#ifdef USER_DEFINED_COINBASE
    // userId -> userCoinbaseInfo
    std::unordered_map<int32_t, string> idCoinbaseInfos_;
    int64_t lastTime_;
#endif

    thread threadUpdate_;
  };

  //--------------------
  atomic<bool> running_;

  bool caseInsensitive_;
  bool stripUserSuffix_;
  string userSuffixSeparator_;

  vector<ChainVars> chains_;
  StratumServer *server_;

  shared_ptr<Zookeeper> zk_;
  string zkUserChainMapDir_;
  bool enableAutoReg_;
  string zkAutoRegWatchDir_;
  uint autoRegMaxPendingUsers_;
  std::set<string> autoRegPendingUsers_;
  std::mutex autoRegPendingUsersLock_;

  std::shared_timed_mutex nameChainlock_;
  // username -> chainId
  std::unordered_map<string, size_t> nameChains_;

  void runThreadUpdate(size_t chainId);
  int32_t incrementalUpdateUsers(size_t chainId);

  bool getChainIdFromZookeeper(const string &userName, size_t &chainId);
  void setZkReconnectHandle();
  static void handleSwitchChainEvent(
      zhandle_t *zh, int type, int state, const char *path, void *pUserInfo);
  static void handleAutoRegEvent(
      zhandle_t *zh, int type, int state, const char *path, void *pUserInfo);

public:
  UserInfo(StratumServer *server, const libconfig::Config &config);
  ~UserInfo();

  void stop();
  bool setupThreads();

  void regularUserName(string &userName);

  // Get chain id by user name.
  //
  // It will first look for nameChains_ and (TODO) zookeeper nodes.
  //
  // If the user is not in nameChains_, look up each chain's nameIds_ and
  // return the first one's id that find the user.
  //
  // If only one chain, chainId=0 and true will always be returned.
  bool getChainId(string userName, size_t &chainId);
  int32_t getUserId(size_t chainId, string userName);
#ifdef USER_DEFINED_COINBASE
  string getCoinbaseInfo(size_t chainId, int32_t userId);
#endif

  bool autoRegEnabled() const { return enableAutoReg_; }
  bool tryAutoReg(string userName, uint32_t sessionId, string fullWorkerName);
};
