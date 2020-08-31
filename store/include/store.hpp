#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

#include <string>

using namespace std;
using namespace eosio;

#define  SUPER_ADMIN      "superadm.mta"_n  // 超级管理员
#define  ADMIN            "admin.mta"_n     // administrator
#define  MGR_ADMIN        "mgr.mta"_n       // Mgr Admin
#define  TOKEN_ACCOUNT    "token.mta"_n     // token账户
#define  HDD_ACCOUNT      "hdd.mta"_n       // 购买hdds时接收token的账户
#define  FORFEIT_ACCOUNT  "forfeit.mta"_n   // 罚金账户
#define  POOL_ADMIN       "pool.mta"_n      // 矿池管理员
#define  POOL_ADMIN2      "pool.mta"_n      // 矿池管理员


/**
 * 存储合约
 */
class [[eosio::contract("store")]] store : public contract {
   public:
      using contract::contract;

      static constexpr symbol CORE_SYMBOL = symbol(symbol_code("MTA"), 4);

      /**
       * 重置数据，测试时使用
       */
      [[eosio::action]]
      void sysreset();

      /**********************************************************************************************
       *                                                                                            *
       *                                            系统参数                                         *
       *                                                                                            *
       *********************************************************************************************/

      /**
       * 初始化系统
       */
      [[eosio::action]]
      void sysinit();

      /**
       * 设置hdd价格
       */
      [[eosio::action]]
      void sethddprice( uint64_t price );

      /**
       * 设置MTA价格
       */
      [[eosio::action]]
      void settokprice( uint64_t price, uint8_t acc_type );

      /**
       * 设置空间和token的比率
       */
      [[eosio::action]]
      void setrate( int64_t rate );

      /**
       * 设置去重比率
       */
      [[eosio::action]]
      void setdrratio( uint64_t ratio, uint8_t acc_type );

      /**
       * 去重分配系数
       */
      [[eosio::action]]
      void setdrdratio( uint64_t ratio );

      /**
       * 修改hdd供应量，添加hdds供应量
       */
      [[eosio::action]]
      void addhddcnt( int64_t count, uint8_t acc_type );


      /**********************************************************************************************
       *                                                                                            *
       *                                            使用存储                                         *
       *                                                                                            *
       *********************************************************************************************/

      /**
       * 购买hdd
       */
      [[eosio::action]]
      void buyhdd( const name& from, const name& receiver, int64_t amount, const string& memo );

      /**
       * 结算hdds的余额
       */
      [[eosio::action]]
      void getbalance( const name& user, uint8_t acc_type, const name& caller );

      /**
       * 设置存储周期费用
       */
      [[eosio::action]]
      void sethfee( const name& user, int64_t fee, const name& caller );

      /**
       * 扣除初始存储费用
       */
      [[eosio::action]]
      void subbalance( const name& user, int64_t amount, uint8_t acc_type, const name& caller );

      /**
       * 添加用户占用空间
       */
      [[eosio::action]]
      void addhspace( const name& user, uint64_t space, const name& caller );

      /**
       * 减少用户占用空间
       */
      [[eosio::action]]
      void subhspace( const name& user, uint64_t space, const name& caller );


      /**********************************************************************************************
       *                                                                                            *
       *                                            矿机相关                                         *
       *                                                                                            *
       *********************************************************************************************/

      /**
       * 存储抵押
       */
      [[eosio::action]]
      void paydeppool( const name& user, asset quant );

      /**
       * 赎回抵押
       */
      [[eosio::action]]
      void unpaydeppool( const name& user, asset quant );

      /**
       * 更新hddm收益
       */
      [[eosio::action]]
      void calcprofit( const name& user );

      /**
       * 用户出售hdd
       */
      [[eosio::action]]
      void sellhdd( const name& user, int64_t amount, const string& memo );
      
      /**
       * 预采购矿机空间
       */
      [[eosio::action]]
      void addmprofit( const name& owner, uint64_t minerid, uint64_t space, const name& caller );

      /**
       * 添加矿机
       */
      [[eosio::action]]
      void newminer( uint64_t minerid, const name& adminacc, const name& dep_acc, asset dep_amount );

      /**
       * 删除矿机
       */
      [[eosio::action]]
      void delminer( uint64_t minerid, uint8_t acc_type, const name& caller );

      /**
       * 矿机更新hddm累计收益
       */
      [[eosio::action]]
      void calcmbalance( const name& owner, uint64_t minerid );

      /**
       * 矿机状态更改为不活跃，无收益
       */
      [[eosio::action]]
      void mdeactive( const name& owner, uint64_t minerid, const name& caller );

      /**
       * 矿机状态更改为活跃矿机，有收益
       */
      [[eosio::action]]
      void mactive( const name& owner, uint64_t minerid, const name& caller );

      /**
       * 矿机修改管理员账号
       */
      [[eosio::action]]
      void mchgadminacc( uint64_t minerid, const name& new_adminacc );

      /**
       * 矿机修改收益账号
       */
      [[eosio::action]]
      void mchgowneracc( uint64_t minerid, const name& new_owneracc );

      /**
       * 矿机修改抵押账号
       */
      [[eosio::action]]
      void mchgdepacc( uint64_t minerid, const name& new_depacc );

      /**
       * 矿机修改所属矿池
       */
      [[eosio::action]]
      void mchgstrpool( uint64_t minerid, const name& new_poolid );

      /**
       * 矿机修改存储空间
       */
      [[eosio::action]]
      void mchgspace( uint64_t minerid, uint64_t max_space );

      /**
       * 矿机修改押金
       */
      [[eosio::action]]
      void chgdeposit( const name& user, uint64_t minerid, bool is_increase, asset quant );

      /**
       * 矿机扣除罚金
       */
      [[eosio::action]]
      void payforfeit( const name& user, uint64_t minerid, asset quant, uint8_t acc_type, const name& caller );


      /**********************************************************************************************
       *                                                                                            *
       *                                            矿池相关                                         *
       *                                                                                            *
       *********************************************************************************************/

      /**
       * 注册矿池
       */
      [[eosio::action]]
      void regstrpool( const name& pool_id, const name& pool_owner );

      /**
       * 删除矿池
       */
      [[eosio::action]]
      void delstrpool( const name& pool_id );

      /**
       * 修改矿池配额
       */
      [[eosio::action]]
      void chgpoolspace( const name& pool_id, bool is_increase, uint64_t delta_space );

      /**
       * 添加矿机到矿池
       */
      [[eosio::action]]
      void addm2pool( uint64_t minerid, const name& pool_id, const name& minerowner, uint64_t max_space );


      

      // 获取抵押金额
      static asset get_deposit( const name& store_contract_account, const name& owner )
      {
         deposits_table deposits( store_contract_account, store_contract_account.value );
         const auto& deposit = deposits.find( owner.value );
         if ( deposit != deposits.end()) {
            return deposit->deposit_total;
         } else {
            return asset( 0, CORE_SYMBOL );
         }
      }

      // 测试用
      using sysreset_action    = action_wrapper<"sysreset"_n, &store::sysreset>;

      // 系统设置
      using sysinit_action      = action_wrapper<"sysinit"_n, &store::sysinit>;
      using sethddprice_action  = action_wrapper<"sethddprice"_n, &store::sethddprice>;
      using settokprice_action  = action_wrapper<"settokprice"_n, &store::settokprice>;
      using setrate_action      = action_wrapper<"setrate"_n, &store::setrate>;
      using setdrratio_action   = action_wrapper<"setdrratio"_n, &store::setdrratio>;
      using setdrdratio_action  = action_wrapper<"setdrdratio"_n, &store::setdrdratio>;
      using addhddcnt_action    = action_wrapper<"addhddcnt"_n, &store::addhddcnt>;

      // 用户
      using buyhdd_action       = action_wrapper<"buyhdd"_n, &store::buyhdd>;
      using getbalance_action   = action_wrapper<"getbalance"_n, &store::getbalance>;
      using sethfee_action      = action_wrapper<"sethfee"_n, &store::sethfee>;
      using subbalance_action   = action_wrapper<"subbalance"_n, &store::subbalance>;
      using addhspace_action    = action_wrapper<"addhspace"_n, &store::addhspace>;
      using subhspace_action    = action_wrapper<"subhspace"_n, &store::subhspace>;
      using paydeppool_action   = action_wrapper<"paydeppool"_n, &store::paydeppool>;
      using unpaydeppool_action = action_wrapper<"unpaydeppool"_n, &store::unpaydeppool>;
      using calcprofit_action   = action_wrapper<"calcprofit"_n, &store::calcprofit>;

      // 矿工
      using sellhdd_action      = action_wrapper<"sellhdd"_n, &store::sellhdd>;
      using addmprofit_action   = action_wrapper<"addmprofit"_n, &store::addmprofit>;
      using newminer_action     = action_wrapper<"newminer"_n, &store::newminer>;
      using delminer_action     = action_wrapper<"delminer"_n, &store::delminer>;
      using calcmbalance_action = action_wrapper<"calcmbalance"_n, &store::calcmbalance>;
      using mdeactive_action    = action_wrapper<"mdeactive"_n, &store::mdeactive>;
      using mactive_action      = action_wrapper<"mactive"_n, &store::mactive>;
      using mchgadminacc_action = action_wrapper<"mchgadminacc"_n, &store::mchgadminacc>;
      using mchgowneracc_action = action_wrapper<"mchgowneracc"_n, &store::mchgowneracc>;
      using mchgstrpool_action  = action_wrapper<"mchgstrpool"_n, &store::mchgstrpool>;
      using mchgspace_action    = action_wrapper<"mchgspace"_n, &store::mchgspace>;
      // using paydeposit_action   = action_wrapper<"paydeposit"_n, &store::paydeposit>;
      using chgdeposit_action   = action_wrapper<"chgdeposit"_n, &store::chgdeposit>;
      using payforfeit_action   = action_wrapper<"payforfeit"_n, &store::payforfeit>;
      using mchgdepacc_action   = action_wrapper<"mchgdepacc"_n, &store::mchgdepacc>;

      // 矿池
      using regstrpool_action   = action_wrapper<"regstrpool"_n, &store::regstrpool>;
      using delstrpool_action   = action_wrapper<"delstrpool"_n, &store::delstrpool>;
      using chgpoolspace_action = action_wrapper<"chgpoolspace"_n, &store::chgpoolspace>;
      using addm2pool_action    = action_wrapper<"addm2pool"_n, &store::addm2pool>;

   private:

      

      /**
       * 系统设置信息
       */
      struct [[eosio::table]] sysinfo {
         name      admin = "store.mta"_n;                       // 管理员账号
         uint64_t  hdd_price = 5760;                            // hdd价格
         uint64_t  token_price = 8000;                          // 代币价格
         uint64_t  rate = 400;                                  // token和空间兑换比率
         uint64_t  dup_remove_ratio = 10000;                    // 去重系数
         uint64_t  dup_remove_dist_ratio = 10000;               // 去重分配系数
         uint64_t  miner_count = 0;                             // 矿机统计
         uint64_t  user_count = 0;                              // 存储用户统计
         int64_t   hdd_counter = 2 * 1024 * 1024 * 100000000ll; // 系统中hdd余额
      };
      typedef singleton< "sysinfo"_n, sysinfo > sysinfo_singleton;

      /**
       * 用户表
       * - owner 用户账户
       * - hdds 存储用hdd余额
       * - usedspace 使用空间
       * - hdds_last_update_time 上次更新hdds时间
       * - hdds_per_cycle_fee 存储周期费用
       * - hddm 挖矿挖到的hdd余额
       * - prod_space 生产空间
       * - hddm_per_cycle_profit 收益周期费用
       * - hddm_last_update_time 上次更新hddm时间
       */
      struct [[eosio::table]] user {
        name        owner;

        int64_t     hdds = 0;
        uint64_t    used_space = 0;
        uint64_t    hdds_per_cycle_fee = 0;
        uint64_t    hdds_last_update_time;

        int64_t     hddm = 0;
        uint64_t    prod_space = 0;
        uint64_t    hddm_per_cycle_profit = 0;
        uint64_t    hddm_last_update_time;

        uint64_t primary_key() const { return owner.value; }
      };
      typedef multi_index< "users"_n, user> users_table;

      /**
       * 抵押表
       * - owner 用户账户名
       * - deposit_total 总抵押
       * - deposit_used 已使用抵押
       * - deposit_his 累计抵押金额
       */
      struct [[eosio::table]] deposit {
        name       owner;
        asset      deposit_total;
        asset      deposit_used = asset(0, CORE_SYMBOL);
        asset      deposit_his;

        uint64_t primary_key() const { return owner.value; }
      };
      typedef multi_index< "deposits"_n, deposit> deposits_table;

      /**
       * 矿池表
       * - pid 矿池id
       * - admin 矿池管理员
       * - max_space 配额
       * - prod_space 已使用配额
       */
      struct [[eosio::table]] store_pool {
         name            id;
         name            owner;
         uint64_t        max_space = 0;
         uint64_t        prod_space = 0;
         uint8_t         pool_type = 0;              // 暂时没用

         uint64_t primary_key() const { return id.value; }
         uint64_t by_owner()  const { return owner.value; }
      };
      typedef multi_index< "storepools"_n, store_pool,
         indexed_by< "owner"_n, const_mem_fun<store_pool, uint64_t, &store_pool::by_owner> >
      > store_pools_table;

      /**
       * 矿机表
       * - mid 矿机id
       * - owner 收益账号
       * - admin 管理员账号
       * - depacc 抵押账号
       * - deposit 抵押的资产
       * - poolid 所属矿池id
       * - max_space 最大空间
       * - space_left 剩余空间
       * - hddm_per_cycle_profit 周期收益
       * - hddm_last_update_time 最后一次计算收益时间
       * - total_profit 累计收益
       */
      struct [[eosio::table]] miner {
         uint64_t    id;
         name        owner;
         name        admin;
         name        pool_id;
         
         name        depacc;
         asset       deposit = asset(0, CORE_SYMBOL);
         asset       deposit_total = asset(0, CORE_SYMBOL);

         uint64_t    prod_space = 0;
         uint64_t    max_space = 0;

         uint64_t    hddm_per_cycle_profit = 0;
         uint64_t    hddm_last_update_time;
         uint64_t    total_profit = 0;
         
         uint64_t  primary_key() const { return id; }
         uint64_t  by_owner()  const { return owner.value; }
         uint64_t  by_admin()  const { return admin.value; }
         uint64_t  by_depacc() const { return depacc.value; }
         uint64_t  by_poolid() const { return pool_id.value; }
      };
      typedef multi_index< "miners"_n, miner,
         indexed_by< "owner"_n, const_mem_fun<miner, uint64_t, &miner::by_owner> >,
         indexed_by< "admin"_n, const_mem_fun<miner, uint64_t, &miner::by_admin> >,
         indexed_by< "poolid"_n, const_mem_fun<miner, uint64_t, &miner::by_poolid> >
      > miners_table;



      /**
       * 系统转账
       */
      void systransfer( const name& from, const name& to, const asset& quantity, const string& memo );

      /**
       * 开通账户
       */
      void create_user( const name& acc, const name& ram_payer );

      // 验证管理员账户
      void check_admin_account( name admin_acc, uint64_t id, bool isCheckId );

      // 结算hdd余额
      void update_hdd_balance( users_table& users, const name& acc, bool is_hdds );

      // 计算hdd余额
      int64_t calculate_balance( int64_t oldbalance, int64_t hdds_per_cycle_fee, int64_t hddm_per_cycle_profit, uint64_t last_update_time, uint64_t current_time );

      // 更新矿机收益
      void update_miner_hddm_balance( miners_table& miners, uint64_t minerid );

      // 获取矿池所有者
      name get_miner_pool_owner( name poolid );

      // 抵押是否足够
      bool is_deposit_enough( asset deposit, uint64_t space ) const;

      // 修改抵押
      void change_deposit_total( const name& owner, bool is_add, asset quant );
};
