#include <store.hpp>
#include <token.hpp>

const uint64_t hours_in_one_day = 24;
const uint64_t minutes_in_one_day = hours_in_one_day * 60;
const uint64_t milliseconds_in_one_day = minutes_in_one_day * 60 * 1000;
const uint64_t milliseconds_in_one_year = milliseconds_in_one_day * 365;

const uint64_t fee_cycle = milliseconds_in_one_day; // 计费周期毫秒为单位


// 以下空间量按照16k一个分片大小为单位
const uint64_t one_gb = 64 * 1024;                                         // 1GB
const uint64_t max_user_space = one_gb * 1024 * uint64_t(1024 * 500);      // 500P 最大用户存储空间量
const uint64_t max_profit_space = one_gb * 1024 * uint64_t(1024 * 500);    // 500P 收益账号最大的生产空间上限
const uint64_t max_pool_space = one_gb * 1024 * uint64_t(1024 * 500);      // 500P 矿池配额最大上限
const uint64_t max_miner_space = one_gb * uint64_t(1024 * 100);            // 100T 单个矿机最大的物理空间
const uint64_t min_miner_space = 100 * one_gb;                             // 100G 单个矿机最小的物理空间    
const int64_t  max_buy_sell_hdd_amount = 2* 1024 * 1024 * 100000000ll;     // 2P   单次买卖最大的HDD数量   
const int64_t  min_buy_hdd_amount = 2 * 100000000ll;                       // 2    单次购买的最小的HDD数量  

// 管理员账户列表
const name admins[5] = {
  "admin1.mta"_n,
  "admin2.mta"_n,
  "admin3.mta"_n,
  "admin4.mta"_n,
  "admin5.mta"_n,
};


static constexpr int64_t max_hdd_amount = (1LL << 62) - 1;

// 当前时间毫秒
inline uint64_t current_time() {
  // return block_timestamp();
  return current_time_point().time_since_epoch().count() / 1000;
}

inline bool is_hdd_amount_within_range( int64_t amount )
{
  return -max_hdd_amount <= amount && amount <= max_hdd_amount;
}

// 重置表数据，测试时使用
void store::sysreset()
{
  require_auth( get_self() );

  // 清空表 users_table
  users_table users( get_self(), get_self().value );
  auto user_itr = users.begin();
  while ( user_itr != users.end() ) {
    user_itr = users.erase( user_itr );
  }

  // 清空表 deposits_table
  deposits_table deposits( get_self(), get_self().value );
  auto deposit_itr = deposits.begin();
  while ( deposit_itr != deposits.end() ) {
    deposit_itr = deposits.erase( deposit_itr );
  }

  // 清空表 miners_table
  miners_table miners( get_self(), get_self().value );
  auto miner_itr = miners.begin();
  while ( miner_itr != miners.end() ) {
    miner_itr = miners.erase( miner_itr );
  }

  // 清空表 store_pools_table
  store_pools_table store_pools( get_self(), get_self().value );
  auto store_pool_itr = store_pools.begin();
  while ( store_pool_itr != store_pools.end() ) {
    store_pool_itr = store_pools.erase( store_pool_itr );
  }

  // 清空系统参数
  sysinfo_singleton _sysinfo( get_self(), get_self().value );
  _sysinfo.remove();
}



/**********************************************************************************************
*                                                                                            *
*                                            系统参数                                         *
*                                                                                            *
*********************************************************************************************/

// 初始化系统
void store::sysinit()
{
  require_auth( get_self() );

  // 初始化全局参数
  sysinfo_singleton _sysinfo( get_self(), get_self().value );
  check( !_sysinfo.exists(), "the system is inited, can't init second time" );
  _sysinfo.set( sysinfo{}, get_self() );
}

// 设置hdd价格
void store::sethddprice( uint64_t price )
{
  require_auth( SUPER_ADMIN );

  check( price > 0, "invalid price" );

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  check( sinfo.hdd_price != price, "Can't set same hdd price" );
  sinfo.hdd_price = price;
  sys_info.set( sinfo, get_self() );
}

// 设置token价格
void store::settokprice( uint64_t price, uint8_t acc_type )
{
  if( acc_type == 1 ) {
    require_auth( ADMIN );
  } else if( acc_type == 2 ) {
    require_auth( SUPER_ADMIN );
  } else {
    require_auth( get_self() );
  }

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  check( price > 0, "invalid price" );
  check( sinfo.token_price != price, "Can't set same token price" );
  sinfo.token_price = price;
  sys_info.set( sinfo, get_self() );
}

// 设置空间和token的比率
void store::setrate( int64_t rate )
{
  require_auth( SUPER_ADMIN );

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  check( sinfo.rate != rate, "Can't set same rate" );
  sinfo.rate = rate;
  sys_info.set( sinfo, get_self() );
}

// 设置去重比率
void store::setdrratio( uint64_t ratio, uint8_t acc_type )
{
  if( acc_type == 1 ) {
    require_auth( ADMIN );
  } else if (acc_type == 2) {
    require_auth( SUPER_ADMIN );
  } else {
    require_auth( get_self() );
  }

  check( ratio > 0 && ratio <= 10000, "invalid deduplication distribute ratio" );

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  check( sinfo.dup_remove_ratio != ratio, "Can't set same dup_remove_ratio" );
  sinfo.dup_remove_ratio = ratio;
  sys_info.set( sinfo, get_self() );
}

// 去重分配系数
void store::setdrdratio( uint64_t ratio )
{
  require_auth( get_self() );

  check( ratio >= 10000, "invalid deduplication ratio" );

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  check( sinfo.dup_remove_dist_ratio != ratio, "Can't set same dup_remove_dist_ratio" );
  sinfo.dup_remove_dist_ratio = ratio;
  sys_info.set( sinfo, get_self() );
}

// 修改hdd供应量，添加hdds供应量
void store::addhddcnt( int64_t count, uint8_t acc_type )
{
  if( acc_type == 1 ) {
    require_auth( POOL_ADMIN );
  } else if ( acc_type == 2 ) {
    require_auth( POOL_ADMIN2 );
  } else {
    require_auth( get_self() );
  }

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  sinfo.hdd_counter += count;
  sys_info.set( sinfo, get_self() );
}



/**********************************************************************************************
 *                                                                                            *
 *                                            使用存储                                         *
 *                                                                                            *
 *********************************************************************************************/

// 购买hdd
void store::buyhdd( const name& from, const name& receiver, int64_t amount, const string& memo )
{
  require_auth( from );

  // 1.验证
  check( is_account( from ), "user not a account" );
  check( is_account( receiver ), "receiver not a account" );
  check( is_account( HDD_ACCOUNT ), "to not a account" );
  check( amount >= min_buy_hdd_amount, "amount too low" );
  check( amount <= max_buy_sell_hdd_amount, "exceed single purchase volume" );

  check( is_hdd_amount_within_range( amount ), "magnitude of amount must be less than 2^62" );

  // 2.获取相关配置
  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  check( sinfo.hdd_counter >= amount, "hdd_counter overdrawn" );
  sinfo.hdd_counter -= amount;
  sys_info.set( sinfo, get_self() );

  int64_t delta = 0;
  if( amount%10000 > 0 ) {
    delta = 1;
  }
  int64_t _token_amount =(int64_t)( ((double)amount/10000) * ((double)sinfo.hdd_price/(double)sinfo.token_price) );
  _token_amount += delta;

  // 3.调用token的方法扣除对应token
  asset quant{ _token_amount, CORE_SYMBOL };
  systransfer( from, HDD_ACCOUNT, quant, "buy " + to_string( amount ) + " hdd" );

  // 4.给当前用户增加相应hdd
  users_table users( get_self(), get_self().value );
  auto user = users.find( receiver.value );

  if ( user == users.end() ) {
    create_user( receiver, from );
    user = users.find( receiver.value );
  }

  users.modify( user, same_payer, [&]( auto &row ){
    row.hdds += amount;
    check( is_hdd_amount_within_range( row.hdds ), "magnitude of user hdds must be less than 2^62" );      
  });
}

// 更新hdds的余接口
void store::getbalance( const name& user, uint8_t acc_type, const name& caller )
{
  check( is_account( user ), "user not a account." );

  if( acc_type == 1 ) {
    require_auth( user );
  } else if ( acc_type == 2 ) {
    check( is_account( caller ), "caller not a account." );
    check_admin_account( caller, user.value, true );
  } else {
    require_auth( get_self() );
  }

  users_table users( get_self(), get_self().value );
  update_hdd_balance( users, user, true );
}

// 设置存储周期费用
void store::sethfee( const name& user, int64_t fee, const name& caller )
{
  check( is_account( user ), "user invalidate" );
  check( is_account( caller ), "caller not an account." );
  check( fee >= 0, "must use positive fee value" );


  users_table users( get_self(), get_self().value );
  auto _user = users.require_find( user.value, "the user is not create" );
  check( fee != _user->hdds_per_cycle_fee, " the fee is the same");

  check_admin_account( caller, user.value, true );

  check( is_hdd_amount_within_range( fee ), "magnitude of fee must be less than 2^62" );      

  // 更新hdds余额
  update_hdd_balance( users, user, true );

  users.modify( _user, same_payer, [&]( auto &row ) {
    row.hdds_per_cycle_fee = fee;
  });
}

// 扣除初始存储费用
void store::subbalance( const name& user, int64_t balance, uint8_t acc_type, const name& caller )
{
  check( is_account( user ), "user invalidate" );

  if( acc_type == 2 ) {
    check( is_account( caller ), "caller not a account." );
    check_admin_account( caller, user.value, false );
  } else {
    require_auth( get_self() );
  }

  check( is_hdd_amount_within_range( balance ), "magnitude of hddbalance must be less than 2^62" );  
  check( balance >= 0, "must use positive balance value" );

  users_table users( get_self(), get_self().value );
  auto _user = users.require_find( user.value, "user not exists in users table" );

  check_admin_account( caller, user.value, true );

  users.modify( _user, same_payer, [&]( auto &row ) {
    row.hdds -= balance;
    check( is_hdd_amount_within_range( row.hdds ), "magnitude of user hdds must be less than 2^62" );
  });
}

// 添加用户占用空间
void store::addhspace( const name& user, uint64_t space, const name& caller )
{
  check( is_account( user ), "user invalidate" );
  check( is_account( caller ), "caller not an account." );

  users_table users( get_self(), get_self().value );
  auto _user = users.require_find( user.value, "user not exists in users table" );

  check_admin_account( caller, user.value, true );

  users.modify( _user, same_payer, [&]( auto &row ) {
    row.used_space += space;
    check( row.used_space <= max_user_space, "overflow max_userspace" );
  });
}

// 减少用户占用空间
void store::subhspace( const name& user, uint64_t space, const name& caller )
{
  check( is_account( user ), "user invalidate" );
  check( is_account( caller ), "caller not an account." );

  users_table users( get_self(), get_self().value );
  auto _user = users.require_find( user.value, "user not exists in users table" );

  check_admin_account( caller, user.value, true );

  users.modify( _user, same_payer, [&]( auto &row ) {
    check(row.used_space >= space , "overdraw user hdd_space");
    row.used_space -= space;
  });
}



 /**********************************************************************************************
  *                                                                                            *
  *                                            矿机相关                                         *
  *                                                                                            *
  *********************************************************************************************/

// 添加矿机并支付押金
void store::newminer( uint64_t minerid, const name& adminacc, const name& dep_acc, asset dep_amount )
{
  require_auth( dep_acc ); // 抵押账号签名

  check( is_account( adminacc ), "adminacc invalidate" );
  check( is_account( dep_acc ), "dep_acc invalidate" );
  check( dep_amount.amount > 0, "must use positive dep_amount" );

  miners_table miners( get_self(), get_self().value );
  auto existing = miners.find( minerid );
  check(existing == miners.end(), "miner already registered");

  miners.emplace( dep_acc, [&]( auto &row ) {      
    row.id             = minerid;
    row.admin          = adminacc;
    row.depacc         = dep_acc;
  });

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  sinfo.miner_count += 1;
  sys_info.set( sinfo, get_self() );

  // 支付押金 另一个合约方法
  chgdeposit( dep_acc, minerid, true, dep_amount );
}

// 删除矿机
void store::delminer( uint64_t minerid, uint8_t acc_type, const name& caller )
{
  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not exist in miners table" );

  if( acc_type == 1 ) {
    check( is_account( caller ), "caller not a account." );
    check_admin_account( caller, minerid, true );
    check(1 == 2, "not support");
  } else if( acc_type == 2 ) {
    require_auth( miner->admin );
    check(1 == 2, "not support");
  } else {
    require_auth( SUPER_ADMIN );
  }

  // 如果抵押过了，返还抵押部分
  deposits_table deposits( get_self(), get_self().value );
  auto depacc = deposits.find( miner->depacc.value );
  if( depacc != deposits.end() ) {
    deposits.modify( depacc, same_payer, [&]( auto& row ) {
      row.deposit_used -= miner->deposit;
      if( row.deposit_used.amount <= 0 ) {
        row.deposit_used = asset(0, CORE_SYMBOL);
      }
    });    
  }

  //扣除该矿机的收益账号的周期收益
  if( miner->owner.value != 0 ) {
    users_table users( get_self(), get_self().value );
    auto user = users.find( miner->owner.value );
    if( user != users.end() ) {
      update_hdd_balance( users, miner->owner, false );
      update_miner_hddm_balance( miners, minerid );
      users.modify( user, same_payer, [&]( auto& row ) {
        row.prod_space -= miner->prod_space;
        row.hddm_per_cycle_profit -= miner->hddm_per_cycle_profit;
      });
    }
  }

  //归还空间到storepool
  if( miner->pool_id.value != 0 ) {
    store_pools_table store_pools( get_self(), get_self().value );
    auto store_pool = store_pools.find( miner->pool_id.value );
    if( store_pool != store_pools.end() ) {
      store_pools.modify( store_pool, same_payer, [&]( auto &row ) {
        row.prod_space -= miner->max_space;
      });  
    }
  }

  //删除该矿机信息
  miners.erase( miner );

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  sinfo.miner_count -= 1;
  sys_info.set( sinfo, get_self() );
}

// 存储网抵押
void store::paydeppool( const name& user, asset quant )
{
  require_auth( user );

  bool is_frozen = token::is_frozen( TOKEN_ACCOUNT, user );
  check( !is_frozen, "frozen user can not create deposit pool" );

  check( is_account( user ), "user is not an account.");
  check( quant.symbol == CORE_SYMBOL, "must use core asset to deposit.");
  check( quant.amount > 0, "must use positive quant" );

  //check if user has enough YTA balance for deposit
  auto balance   = token::get_balance( TOKEN_ACCOUNT, user, CORE_SYMBOL.code() );
  deposits_table deposits( get_self(), get_self().value ); // 实例化抵押表
  auto deposit = deposits.find( user.value );
  
  //插入或者更新抵押表
  if ( deposit == deposits.end() ) {
    check( balance.amount >= quant.amount, "user balance not enough." );
    deposits.emplace( user, [&]( auto& row ){
      row.owner = user;
      row.deposit_total = quant;
      row.deposit_his = quant;
    });
  } else {
    asset deposit_total = deposit->deposit_total + quant;
    check( balance.amount >= deposit_total.amount, "user balance not enough." );
    deposits.modify( deposit, same_payer, [&]( auto& row ) {
      row.deposit_total += quant;
      row.deposit_his += quant;
    });
  }

}

// 赎回存储抵押
void store::unpaydeppool( const name& user, asset quant )
{
  require_auth( user );

  check( quant.symbol == CORE_SYMBOL, "must use core asset for hdd deposit." );
  check( quant.amount > 0, "must use positive quant" );

  bool is_frozen = token::is_frozen( TOKEN_ACCOUNT, user );
  check( !is_frozen, "user is frozen" );

  deposits_table deposits( get_self(), get_self().value );
  auto deposit = deposits.require_find( user.value, "no deposit record for this user." );

  check( deposit->deposit_total.amount - deposit->deposit_used.amount >= quant.amount, "free deposit not enough." );
  check( deposit->deposit_total.amount >= quant.amount, "deposit not enough." );
  check( deposit->deposit_his.amount >= quant.amount, "deposit not enough." );

  // 修改抵押表
  deposits.modify( deposit, same_payer, [&]( auto& row ) {
    row.deposit_total -= quant;
    row.deposit_his -= quant;
  });
}

// 添加矿机到矿池
void store::addm2pool( uint64_t minerid, const name& pool_id, const name& minerowner, uint64_t max_space )
{
  check( is_account( minerowner ), "minerowner invalidate" );

  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );

  store_pools_table store_pools( get_self(), get_self().value );
  auto store_pool = store_pools.require_find( pool_id.value, "storepool not registered" );

  require_auth( miner->admin );
  require_auth( store_pool->owner );
  
  check( miner->pool_id.value == 0, "miner already join to the pool:" + pool_id.to_string() );  
  check( max_space <= max_miner_space, "miner max_space overflow" );  
  check( max_space >= min_miner_space, "miner max_space underflow" );  
  check( store_pool->max_space - store_pool->prod_space >= max_space, "pool space not enough" );

  // TODO: check miner deposit and max_space
  check( is_deposit_enough( miner->deposit, max_space ), "deposit not enough for miner's max_space -- addm2pool" );
  //--- check miner deposit and max_space

  // 修改矿机信息的矿池字段
  miners.modify( miner, same_payer, [&]( auto &row ) {
    row.pool_id = pool_id;
    row.owner = minerowner;
    row.max_space = max_space;
    row.hddm_last_update_time = current_time();
  });  

  // 扣除矿池配额
  store_pools.modify( store_pool, same_payer, [&]( auto &row ) {
    row.prod_space += max_space;
  });

  // 如果收益账户为开户则开户
  users_table users( get_self(), get_self().value );
  auto existing = users.find( minerowner.value );
  if ( existing == users.end() ) {
    create_user( minerowner, miner->admin );
  }
}


// 矿机修改所属矿池
void store::mchgstrpool( uint64_t minerid, const name& new_poolid )
{
  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );

  // 归还旧矿池空间
  store_pools_table store_pools( get_self(), get_self().value );
  auto store_pool_old = store_pools.require_find( miner->pool_id.value, "original storepool not registered" );
  store_pools.modify( store_pool_old, same_payer, [&]( auto &row ) {
    check(row.prod_space >= miner->max_space, "over space");
    row.prod_space -= miner->max_space;
    // if(row.prod_space <= 0) {
    //   row.prod_space = 0;
    // }
  }); 

  //清空miner表中该矿机的矿池id
  miners.modify( miner, same_payer, [&]( auto &row ) {
    row.pool_id.value = 0;
  });

  //加入新矿池
  addm2pool( minerid, new_poolid, miner->owner, miner->max_space );
}

// 更新hddm收益
void store::calcprofit( const name& user )
{
  require_auth( user );

  users_table users( get_self(), get_self().value );
  auto _user = users.require_find( user.value, "user not exists in users table" );

  update_hdd_balance( users, user, false );
}

// 用户出售hdd
void store::sellhdd( const name& user, int64_t amount, const string& memo )
{
  require_auth( user );

  check( amount > 0, "cannot sell negative hdd amount" );
  check( amount <= max_buy_sell_hdd_amount, "exceed single sale volume" );
  check( is_hdd_amount_within_range( amount ), "magnitude of user hdd amount must be less than 2^62" );      


  users_table users( get_self(), get_self().value );
  auto _user = users.require_find( user.value, "user not exists in users table" );
  check( _user->hddm >= amount, "hdd overdrawn." );

  users.modify( _user, same_payer, [&]( auto &row ) {
    row.hddm -= amount;
    check( is_hdd_amount_within_range( row.hddm ), "magnitude of user hddm must be less than 2^62" );      
  });

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();

  int64_t _token_amount =(int64_t)( ( (double)amount/10000) * ((double)sinfo.hdd_price/(double)sinfo.token_price) * ((double)sinfo.dup_remove_ratio/10000) * ((double)sinfo.dup_remove_dist_ratio/10000) );

  // 给用户转相应的token
  asset quant{ _token_amount, CORE_SYMBOL };
  systransfer( HDD_ACCOUNT, user, quant, "sell hddm" );
}

// 采购矿机空间 owner 参数多余？
void store::addmprofit( const name& owner, uint64_t minerid, uint64_t space, const name& caller )
{
  ((void)owner);
  check( is_account( caller ), "caller not an account." );
  check_admin_account( caller, minerid, true );

  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );
  check( space + miner->prod_space <= miner->max_space, "exceed max space" );
  check( miner->owner == owner, "invalid owner");

  miners.modify( miner, same_payer, [&]( auto &row ) {
    row.prod_space += space;
    row.hddm_per_cycle_profit = (int64_t)((double)(row.prod_space / (double)one_gb) * ((double)fee_cycle / (double)milliseconds_in_one_year) * 100000000);
  });
  
  users_table users( get_self(), get_self().value );
  auto user = users.require_find( miner->owner.value, "owner not exists in users table." );

  // 更新hddm余额
  update_hdd_balance( users, miner->owner, false );

  users.modify( user, same_payer, [&]( auto &row ) {
    row.prod_space += space;
    //每周期收益 += (生产空间/1GB）*（记账周期/ 1年）
    row.hddm_per_cycle_profit = (int64_t)((double)(row.prod_space / (double)one_gb) * ((double)fee_cycle / (double)milliseconds_in_one_year) * 100000000);
  });
}

// 矿机更新hddm累计收益
void store::calcmbalance( const name& owner, uint64_t minerid )
{
  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );

  require_auth( miner->owner );

  miners.modify( miner, get_self(), [&]( auto &row ) {
    uint64_t tmp_t = current_time();
    row.total_profit = calculate_balance( miner->total_profit , 0, miner->hddm_per_cycle_profit, miner->hddm_last_update_time, tmp_t);
    row.hddm_last_update_time = tmp_t;
  });
}

// 矿机状态更改为不活跃，无收益 owner多余
void store::mdeactive( const name& owner, uint64_t minerid, const name& caller )
{
  (void(owner));
  // check( is_account( owner ), "owner invalidate" );
  check( is_account( caller ), "caller not an account." );

  check_admin_account( caller, minerid, true );

  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );
  check( miner->hddm_per_cycle_profit > 0 && miner->prod_space > 0, "Can't deactive a not active miner" );

  // 更新收益账号hddm余额,减少周期收益
  users_table users( get_self(), get_self().value );
  auto user = users.require_find( miner->owner.value, "the miner's owner is not exist" );
  update_hdd_balance( users, miner->owner, false );
  users.modify( user, same_payer, [&]( auto& row ) {
    row.hddm_per_cycle_profit -= miner->hddm_per_cycle_profit;
    row.prod_space -= miner->prod_space;
  });

  // 更新矿机收益，周期收益设为0
  update_miner_hddm_balance( miners, minerid );
  miners.modify( miner, same_payer, [&]( auto &row ) {
    row.hddm_per_cycle_profit = 0;
  });
}

// 矿机状态更改为活跃矿机，有收益 如何区分矿机是否活跃？？？
void store::mactive( const name& owner, uint64_t minerid, const name& caller )
{
  (void(owner));
  // check( is_account( owner ), "owner not an account." );
  check( is_account( caller ), "caller not an account." );

  check_admin_account( caller, minerid, true );

  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );
  check( miner->hddm_per_cycle_profit == 0 && miner->prod_space > 0, "Can't active an active miner" );

  // update_miner_hddm_balance( miners, minerid );
  
  int64_t profit = 0;
  //每周期收益 += (生产空间*数据分片大小/1GB）*（记账周期/ 1年）
  profit = (int64_t)(((double)miner->prod_space / (double)one_gb) * ((double)fee_cycle / (double)milliseconds_in_one_year) * 100000000);

  miners.modify( miner, same_payer , [&]( auto &row ) {
    row.hddm_last_update_time = current_time();
    row.hddm_per_cycle_profit = profit;
  });

  // 更新用户表，周期收益
  users_table users( get_self(), get_self().value );
  auto user = users.find( miner->owner.value );
  update_hdd_balance( users, miner->owner, false );
  users.modify( user, same_payer, [&]( auto& row ) {
    row.hddm_per_cycle_profit += miner->hddm_per_cycle_profit;
    row.prod_space += miner->prod_space;
  });
}

// 矿机修改管理员账号
void store::mchgadminacc( uint64_t minerid, const name& new_adminacc )
{
  check( is_account( new_adminacc ), "new admin is not an account.");

  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );

  require_auth( miner->admin );

  miners.modify( miner, same_payer, [&]( auto &row ) {
    row.admin = new_adminacc;
  });
}

// 矿机修改收益账号
void store::mchgowneracc( uint64_t minerid, const name& new_owneracc )
{
  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );

  check( is_account( new_owneracc ), "new owner is not an account.");
  check( miner->owner.value != 0, "no owner for this miner" );
  check( miner->owner != new_owneracc, "new owner already own this miner" );

  const name& pool_owner = get_miner_pool_owner( miner->pool_id );

  require_auth( miner->admin );
  require_auth( pool_owner );

  uint64_t tmp_t = current_time();
  users_table users( get_self(), get_self().value );

  // 结算旧owner账户当前的收益，并扣除当前矿机的周期收益生产空间
  auto owner_old = users.require_find( miner->owner.value, "the old owner is not exist" );
  update_hdd_balance( users, miner->owner, false );
  users.modify( owner_old, same_payer, [&]( auto& row ) {
    row.hddm_per_cycle_profit -= miner->hddm_per_cycle_profit;
    row.prod_space -= miner->prod_space;
  });

  // 结算新owner账户当前的收益，并增加当前矿机的周期收益生产空间
  auto user_new = users.find( new_owneracc.value );
  if ( user_new == users.end() ) {
    create_user( new_owneracc, miner->admin );
    user_new = users.find( new_owneracc.value );
  } else {
    update_hdd_balance( users, new_owneracc, false );
  }
  users.modify( user_new, same_payer, [&]( auto& row ) {
    row.hddm_per_cycle_profit += miner->hddm_per_cycle_profit;
    row.prod_space += miner->prod_space;
  });

  //变更矿机表的收益账户名称
  miners.modify( miner, get_self(), [&]( auto &row ) {
    row.owner = new_owneracc;
  });
}

// 矿机修改抵押账号
void store::mchgdepacc( uint64_t minerid, const name& new_depacc )
{
  require_auth( new_depacc );

  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );
  check( miner->depacc != new_depacc, "must use different account to change deposit user" );

  deposits_table deposits( get_self(), get_self().value );
  auto depacc_old = deposits.require_find( miner->depacc.value, "no deposit record for original deposit user" );
  auto depacc_new = deposits.require_find( new_depacc.value, "no deposit record for new deposit user" );
  check( depacc_new->deposit_total.amount - depacc_new->deposit_used.amount >= miner->deposit.amount, "new deposit user free deposit not enough" );

  //变更原抵押账户的押金数量
  deposits.modify( depacc_old, same_payer, [&]( auto& row ) {
    row.deposit_used -= miner->deposit;
  });  

  //将矿机的押金数量重新恢复到未扣罚金的初始额度
  miners.modify( miner, same_payer, [&]( auto& row ) {
    row.depacc  = new_depacc;
    row.deposit = row.deposit_total;
  });

  // 添加新账号的已使用押金
  deposits.modify( depacc_new, same_payer, [&]( auto& row ) {
    row.deposit_used += miner->deposit_total;
  });
}

// 矿机修改最大存储空间
void store::mchgspace( uint64_t minerid, uint64_t max_space )
{
  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );
  check( max_space <= max_miner_space, "miner max_space overflow" );  
  check( max_space >= min_miner_space, "miner max_space underflow" );
  check( max_space != miner->max_space, "can't change same space" );

  bool is_add_space;
  uint64_t diff_space;
  if( max_space  > miner->max_space ) {
    is_add_space = true;
    diff_space = max_space - miner->max_space;

    // 剩余抵押是否足够
    check( is_deposit_enough( miner->deposit, max_space ), "miner's deposit not enough for miner's max_space" );
  } else {
    is_add_space = false;
    diff_space = miner->max_space - max_space;
  }

  const name& pool_owner = get_miner_pool_owner( miner->pool_id );

  require_auth( miner->admin );
  require_auth( pool_owner );

  // 修改矿池信息
  store_pools_table store_pools( get_self(), get_self().value );
  auto store_pool = store_pools.require_find( miner->pool_id.value, "storepool not exist" ); 
  store_pools.modify( store_pool, same_payer, [&]( auto &row ) {
    if( is_add_space ) {
      check( row.max_space - row.prod_space >= diff_space, "exceed storepool's max space" );      
      row.prod_space += diff_space;
    } else {
      row.prod_space -= diff_space;
    }
  });

  // 修改矿机信息
  miners.modify( miner, same_payer, [&]( auto &row ) {
    check(row.prod_space <= max_space, "invalid max_space");      
    row.max_space = max_space;
  });
}

// 矿机修改押金
void store::chgdeposit( const name& user, uint64_t minerid, bool is_increase, asset quant )
{
  check( quant.symbol == CORE_SYMBOL, "must use core asset for hdd deposit." );
  check( quant.amount > 0, "must use positive quant" );
  
  miners_table miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );
  check( miner->depacc == user, "must use same account to change deposit." );

  require_auth( miner->depacc ); // need hdd official account to sign this void.

  bool is_frozen = token::is_frozen( TOKEN_ACCOUNT, miner->depacc );
  check( !is_frozen, "miner's depacc is frozen" );

  deposits_table deposits( get_self(), get_self().value );
  auto deposit = deposits.require_find( miner->depacc.value, "no deposit record for this minerid." );

  if( !is_increase ) {
    check( miner->deposit.amount >= quant.amount, "overdrawn deposit." );

    deposits.modify( deposit, same_payer, [&]( auto& row ) {
      row.deposit_used -= quant;
    });
    miners.modify( miner, same_payer, [&]( auto& row ) {
      row.deposit -= quant;
      row.deposit_total -= quant;
    });
  } else {
    check( deposit->deposit_total.amount - deposit->deposit_used.amount >= quant.amount, "free deposit not enough." );
    deposits.modify( deposit, same_payer, [&]( auto& row ) {
      row.deposit_used += quant;
    });
    miners.modify( miner, same_payer, [&]( auto& row ) {
      row.deposit += quant;
      row.deposit_total += quant;
    });
  }

  //--- TODO:最后验证抵押是否足够
  check( is_deposit_enough( miner->deposit, miner->max_space ), "deposit not enough for miner's max_space -- chgdeposit" );
  //--- check miner deposit and max_space
}

// 矿机扣除罚金 user 参数多余
void store::payforfeit( const name& user, uint64_t minerid, asset quant, uint8_t acc_type, const name& caller )
{
  (void(user));
  if( acc_type == 2 ) {
    check( is_account( caller ), "caller not a account.");
    //check(is_bp_account(caller.value), "caller not a BP account.");
    //require_auth( caller );
    check_admin_account( caller, minerid, true );
  } else {
    require_auth( get_self() );
  }

  // check( is_account( user ), "user is not an account." );
  check( quant.symbol == CORE_SYMBOL, "must use core asset for hdd deposit." );
  check( quant.amount > 0, "must use positive quant" );

  miners_table   miners( get_self(), get_self().value );
  auto miner = miners.require_find( minerid, "minerid not register" );
  check( miner->deposit.amount >= quant.amount, "overdrawn deposit." );

  deposits_table deposits( get_self(), get_self().value );
  auto deposit = deposits.require_find( miner->depacc.value, "no deposit pool record for this miner." );
  
  check( deposit->deposit_used.amount >= quant.amount, "overdrawn deposit." );

  // 扣除矿机押金
  miners.modify( miner, same_payer, [&]( auto& row ) {
    row.deposit.amount -= quant.amount;
  });

  // 跨合约扣除token
  systransfer( miner->depacc, FORFEIT_ACCOUNT, quant, "pay forfeit" );

  // 扣除抵押账号押金
  deposits.modify( deposit, same_payer, [&]( auto& row ) {
    row.deposit_total -= quant;
    row.deposit_used -= quant;
  });
}




/**********************************************************************************************
*                                                                                            *
*                                            矿池相关                                         *
*                                                                                            *
*********************************************************************************************/

// 注册矿池
void store::regstrpool( const name& pool_id, const name& pool_owner )
{
  require_auth( pool_owner );

  store_pools_table store_pools( get_self(), get_self().value );
  auto existing = store_pools.find( pool_id.value );
  check( existing == store_pools.end(), "storepool already registered" );

  store_pools.emplace( pool_owner, [&]( auto &row ) {
    row.id         = pool_id;
    row.owner      = pool_owner;
  });

  // 扣除10个代币
  asset quant( 100000, CORE_SYMBOL );
  systransfer( pool_owner, HDD_ACCOUNT, quant, "pay for creation storepool " + pool_id.to_string() );
}

// 删除矿池
void store::delstrpool( const name& pool_id )
{
  require_auth( POOL_ADMIN );

  store_pools_table store_pools( get_self(), get_self().value );
  auto store_pool = store_pools.require_find( pool_id.value, "the store pool is not registered" );
  check( store_pool->prod_space == 0, "can not delete this storepool." );
  store_pools.erase( store_pool );
}

// 修改矿池配额
void store::chgpoolspace( const name& pool_id, bool is_increase, uint64_t delta_space )
{
  require_auth( POOL_ADMIN );

  check( delta_space > 0, "must use positive delta_space" );

  store_pools_table store_pools( get_self(), get_self().value );
  auto store_pool = store_pools.require_find( pool_id.value, "storepool not exist" );

  uint64_t max_space = 0;
  if( is_increase ) {
    max_space = store_pool->max_space + delta_space;
    check( max_space <= max_pool_space, "execeed max pool space" );
  } else {
    check( store_pool->max_space > delta_space, "overdrawn max_space" );
    max_space = store_pool->max_space - delta_space;
  }

  store_pools.modify( store_pool, same_payer, [&]( auto &row ) {
    check( row.prod_space <= max_space, "invalid max_space" );
    row.max_space = max_space;
  });
}



/**********************************************************************************************
*                                                                                            *
*                                            私有方法                                         *
*                                                                                            *
*********************************************************************************************/

// 验证管理员账号 TODO:待实现
void store::check_admin_account( name admin_acc, uint64_t id, bool isCheckId )
{
  // name admin;
  // admin = admins[id%5];
  // check( admin == admin_acc, "Incorrect administrator account");
  // require_auth( admin );
  require_auth( admin_acc );
}

// 计算余额
int64_t store::calculate_balance( int64_t oldbalance, int64_t hdds_per_cycle_fee, int64_t hddm_per_cycle_profit, uint64_t last_update_time, uint64_t current_time )
{
  uint64_t slot_t = ( current_time - last_update_time ) / 1000ll; // convert to milliseconds
  int64_t new_balance = oldbalance;

  double tick = (double)( (double)slot_t / fee_cycle );
  int64_t delta = (int64_t)( tick * ( hddm_per_cycle_profit - hdds_per_cycle_fee ) );
  new_balance += delta;

  check( is_hdd_amount_within_range( new_balance ), "magnitude of user hdds must be less than 2^62" );

  return new_balance;
}

// 结算用户的hdd
void store::update_hdd_balance( users_table& users, const name& acc, bool is_hdds )
{
  auto user = users.require_find( acc.value, "the user is not create" );
  users.modify( user, same_payer, [&]( auto &row ) {
    uint64_t tmp_t = current_time();
    if ( is_hdds ) {
      row.hdds = calculate_balance( user->hdds, user->hdds_per_cycle_fee, 0, user->hdds_last_update_time, tmp_t );
      row.hdds_last_update_time = tmp_t;
      print("{\"balance\":", row.hdds, "}");
    } else {
      row.hddm = calculate_balance( user->hddm, 0, user->hddm_per_cycle_profit, user->hddm_last_update_time, tmp_t );
      row.hddm_last_update_time = tmp_t;
      print("{\"balance\":", row.hddm, "}");
    }
  });
}

// 更新矿机的hddm
void store::update_miner_hddm_balance( miners_table& miners, uint64_t minerid )
{
  auto miner = miners.require_find( minerid, "minerid not register" );

  miners.modify( miner, same_payer, [&]( auto &row ) {
    uint64_t tmp_t = current_time();
    row.total_profit = calculate_balance( miner->total_profit , 0, miner->hddm_per_cycle_profit, miner->hddm_last_update_time, tmp_t );
    row.hddm_last_update_time = tmp_t;
  });
}

// 获取矿池所有者
name store::get_miner_pool_owner( name pool_id )
{
  store_pools_table store_pools( get_self(), get_self().value );
  auto store_pool = store_pools.require_find( pool_id.value, "the pool_id is not exist" );
  return store_pool->owner;
}

// 抵押是否足够
bool store::is_deposit_enough( asset deposit, uint64_t space ) const
{
  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  int64_t rate = sinfo.rate;
  double drate = ((double)rate) / 100;

  int64_t am = (int64_t)((((double)space) / one_gb) * drate * 10000);
  if (deposit.amount >= am)
    return true;

  return false;
}

// 系统转账
void store::systransfer( const name& from, const name& to, const asset& quantity, const string& memo )
{
  action(
    permission_level{ MGR_ADMIN, "active"_n },              // permission_level,
    TOKEN_ACCOUNT,                                          // code,
    "systransfer"_n,                                        // action,
    std::make_tuple( from, to, quantity, memo )             // data
  ).send();
}

// 修改抵押金额
void store::change_deposit_total( const name& owner, bool is_add, asset quant )
{
  // 扣除抵押账号押金
  deposits_table deposits( get_self(), get_self().value );
  auto deposit = deposits.require_find( owner.value, "no deposit pool record for this user." );
  if ( is_add ) {
    deposits.modify( deposit, same_payer, [&]( auto& row ) {
      row.deposit_total += quant;
    });
  } else {
    check( deposit->deposit_used.amount >= quant.amount, "overdrawn deposit." );
    deposits.modify( deposit, same_payer, [&]( auto& row ) {
      row.deposit_total -= quant;
    });
  }
}

// 开通user账户
void store::create_user( const name& user, const name& ram_payer )
{
  users_table users( get_self(), get_self().value );
  auto existing = users.find( user.value );
  check( existing == users.end(), "the account is already create." );

  users.emplace( ram_payer, [&]( auto &row ){
    row.owner = user;
    row.hdds_last_update_time = current_time();
    row.hddm_last_update_time = current_time();
  });

  sysinfo_singleton sys_info( get_self(), get_self().value );
  auto sinfo = sys_info.get();
  sinfo.user_count += 1;
  sys_info.set( sinfo, get_self() );
}
