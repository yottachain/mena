#include <token.hpp>
#include <store.hpp>

void token::create( const name& issuer, const asset& maximum_supply )
{
   require_auth( get_self() );

   auto sym = maximum_supply.symbol;
   check( sym.is_valid(), "invalid symbol name" );
   check( maximum_supply.is_valid(), "invalid supply");
   check( maximum_supply.amount > 0, "max-supply must be positive");
   check( is_account( issuer ), "issuer account does not exist");

   stats statstable( get_self(), sym.code().raw() );
   auto existing = statstable.find( sym.code().raw() );
   check( existing == statstable.end(), "token with symbol already exists" );

   statstable.emplace( get_self(), [&]( auto& s ) {
      s.supply.symbol = maximum_supply.symbol;
      s.max_supply    = maximum_supply;
      s.issuer        = issuer;
   });
}

void token::issue( const name& to, const asset& quantity, const string& memo )
{
   auto sym = quantity.symbol;
   check( sym.is_valid(), "invalid symbol name" );
   check( is_account( to ), "to account does not exist");
   check( memo.size() <= 256, "memo has more than 256 bytes" );
   
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "This token is not existed when issue." );
   require_auth( st.issuer );

   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must issue positive quantity" );
   check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
   check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.supply += quantity;
   });

   add_balance( to, quantity, st.issuer );
}

void token::transfer( const name& from, const name& to, const asset& quantity, const string& memo )
{
   require_auth( from );

   check( from != to, "cannot transfer to self" );
   check( is_account( to ), "to account does not exist");
   auto sym = quantity.symbol;
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "This token is not existed when tranfer." );

   require_recipient( from );
   require_recipient( to );

   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must transfer positive quantity" );
   check( sym == st.supply.symbol, "symbol precision mismatch" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   sub_balance( from, quantity, false );
   add_balance( to, quantity, from );
}

void token::systransfer( const name& from, const name& to, const asset& quantity, const string& memo )
{
   require_auth( MGR_ADMIN );

   check( from != to, "cannot transfer to self" );
   check( is_account( to ), "to account does not exist");
   auto sym = quantity.symbol;
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "This token is not existed when tranfer." );

   require_recipient( from );
   require_recipient( to );

   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must transfer positive quantity" );
   check( sym == st.supply.symbol, "symbol precision mismatch" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   sub_balance( from, quantity, to == FORFEIT_ACCOUNT );

   add_balance( to, quantity, MGR_ADMIN );
}

void token::sub_balance( const name& account, const asset& value, bool is_force )
{
   accounts _accounts( get_self(), account.value );
   auto sym = value.symbol;
   const auto& from = _accounts.get( sym.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   if ( !is_force ) {
      check( !check_frozen( account ), "account has been frozen" );

      auto lock_asset = get_lock_asset( account, sym );
      check( sym == lock_asset.symbol, "lockasset's symbol or precision mismatch" );
      check( from.balance.amount - lock_asset.amount >= value.amount, "overdrawn balance, " + lock_asset.to_string() + " is locked" );

      auto deposit = store::get_deposit( STORE_ACCOUNT, account );
      check( from.balance.amount - deposit.amount >= value.amount, "overdrawn balance, " + deposit.to_string() + " is deposit" );
   }

   if( from.balance.amount == value.amount ) {
      _accounts.erase( from );
   } else {
      _accounts.modify( from, same_payer, [&]( auto& a ) {
         a.balance -= value;
      });
   }
}

void token::add_balance( const name& account, const asset& value, const name& ram_payer )
{
   accounts _accounts( get_self(), account.value );
   auto to = _accounts.find( value.symbol.code().raw() );
   if( to == _accounts.end() ) {
      _accounts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      _accounts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::setextime( uint64_t time, const symbol& sym )
{
   check( sym.is_valid(), "invalid symbol" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "This token is not existed when setextime." );
   check( st.extime == 0, "The exchanging time has already been setted." );

   require_auth( st.issuer );

   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.extime = time;
   });
}

void token::freezeacc( const name& acc, uint64_t time )
{
   require_auth( TOKEN_ADMIN );

   check( is_account( acc ), "Target account does not exist");

   accfrozens _accfrozens( get_self(), get_self().value );
   auto accfro = _accfrozens.find( acc.value );
   if( accfro == _accfrozens.end() ) {
      _accfrozens.emplace( get_self(), [&](auto &row ) {
         row.user = acc;
         row.time = time;
      });
   } else {
      _accfrozens.modify( accfro, get_self(), [&](auto &row ) {
         row.time = time;
      });
   }
}

void token::unfreezeacc( const name& acc )
{
   require_auth( TOKEN_ADMIN );

   accfrozens _accfrozens( get_self(), get_self().value );
   const auto& accfro = _accfrozens.get( acc.value, "account has not been frozen" );
   _accfrozens.erase( accfro );
}

void token::addaccbig( const name& user, const string& desc )
{
   require_auth( TOKEN_LOCKE );

   check( is_account( user ), "Target account does not exist");

   accbigs _accbigs( get_self(), get_self().value );
   auto itmaccbig = _accbigs.find( user.value );
   check( itmaccbig == _accbigs.end(), "user already registered as big account" );  

   _accbigs.emplace( get_self(), [&](auto &row ) {
      row.user = user;
      row.desc = desc;
   });
}

void token::rmvaccbig( const name& user )
{
   require_auth( TOKEN_LOCKE );

   accbigs _accbigs( get_self(), get_self().value );
   const auto& itmaccbig = _accbigs.get( user.value, "account is not a big account" );
   _accbigs.erase( itmaccbig );
}

void token::addrule( uint64_t lockruleid, const vector<uint64_t>& times, const vector<uint8_t>& pcts, uint8_t base, bool isabsolute, const string& desc ) 
{
   require_auth( TOKEN_LOCKE );

   check( times.size() >= 2, "invalidate size of times array" );
   check( times.size() == pcts.size(), "times and percentage in different size." );

   lockrules _lockrules( get_self(), get_self().value );
   auto existing = _lockrules.find( lockruleid );
   check( existing == _lockrules.end(), "the id already existed in rule table" ); 

   for( size_t i = 0; i < times.size(); i++ ) {
      if( i == 0 ){
         check( pcts[i] >= 0 && pcts[i] <= base, "invalidate lock percentage" );
      } else {
         check( times[i] > times[i-1], "times vector error" );
         check( pcts[i] > pcts[i-1] && pcts[i] <= base, "lock percentage vector error" );
      }
   }

   _lockrules.emplace( get_self(), [&](auto &row ) {
      row.lockruleid   = lockruleid;
      row.times        = times;
      row.pcts         = pcts;
      row.base         = base;
      row.desc         = desc;
      row.isabsolute   = isabsolute;
   });          
}

void token::locktransfer( uint64_t lockruleid, const name& from, const name& to, const asset& quantity, const string& memo ) 
{
   require_auth( from );

   check( quantity.amount > 0, "must locktransfer positive quantity" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   accbigs _accbigs( get_self(), get_self().value );
   auto isbig = _accbigs.find( from.value );
   check( isbig != _accbigs.end(), "from can not locktransfer" );  

   lockrules _lockrules( get_self(), get_self().value );
   auto existing = _lockrules.find( lockruleid );
   check( existing != _lockrules.end(), "lockruleid not existed in rule table" );  

   transfer( from, to, quantity, memo );

   acclocks _acclocks( get_self(), to.value );
   auto itlc = _acclocks.find( lockruleid );
   if( itlc != _acclocks.end() ) {
      _acclocks.modify( itlc, get_self(), [&](auto &row ) {
         row.time = current_time_point().sec_since_epoch();
         row.quantity += quantity;
      });
   } else {
      _acclocks.emplace( get_self(), [&](auto &row ) {
         row.lockruleid  = lockruleid;
         row.quantity    = quantity;
         row.user        = to;
         row.from        = from;
         row.time        = current_time_point().sec_since_epoch();
      });
   }
}

bool token::check_frozen( const name& user )
{
   bool isFrozen = false;
   accfrozens _accfrozens( get_self(), get_self().value );
   auto it = _accfrozens.find( user.value );
   if( it != _accfrozens.end() ) {
      uint64_t current_time = current_time_point().sec_since_epoch(); //seconds
      if( current_time < it->time ) {
         isFrozen = true;
      } else {
         _accfrozens.erase( it );
      }
   }
   return isFrozen;
}

asset token::get_lock_asset( const name& user, const symbol& sym )
{
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed" );
   acclocks _acclock( get_self(), user.value );
   uint64_t curtime = current_time_point().sec_since_epoch(); //seconds

   asset lockasset( 0, sym );

   for( auto it = _acclock.begin(); it != _acclock.end(); it++ ) {
      
      if ( it->quantity.symbol != sym ) {
         continue;
      }

      int64_t amount = it->quantity.amount; 
      uint8_t percent = 0;
      size_t n = 0;

      lockrules _lockrules( get_self(), get_self().value );
      auto itrule = _lockrules.find( it->lockruleid ); // get lock rule
      check( itrule != _lockrules.end(), "lockruleid not existed in rule table" );

      if ( itrule->isabsolute ) { // Absolute time calculation
         for( auto itt = itrule->times.begin(); itt != itrule->times.end(); itt++ ) {
            if( *itt > curtime ) {
               break;
            }
            percent = itrule->pcts[n];
            n++;
         }
      } else if ( st.extime != 0 ) { // Relative time calculation
         for( auto itt = itrule->times.begin(); itt != itrule->times.end(); itt++ ) {
            if( *itt + st.extime > curtime ) {
               break;
            }
            percent = itrule->pcts[n];
            n++; 
         }
      }

      check( percent >= 0 && percent <= itrule->base, "invalidate lock percentage" );
      percent = itrule->base - percent;
      lockasset.amount += (int64_t)(((double)amount / itrule->base)*percent);
   }

   return lockasset;
}
