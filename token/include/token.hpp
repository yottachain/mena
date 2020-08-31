#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>


#include <string>

using namespace std;
using namespace eosio;

#define  TOKEN_ADMIN      "admin.mta"_n       // Freeze administrator
#define  TOKEN_LOCKE      "locker.mta"_n      // Lock Manager
#define  MGR_ADMIN        "mgr.mta"_n         // Mgr Admin
#define  FORFEIT_ACCOUNT  "forfeit.mta"_n   // Forfeit
#define  STORE_ACCOUNT    "store.mta"_n       // Store account

/**
 * token contract defines the structures and actions that allow users to create, issue, and manage
 * tokens on eosio based blockchains.
 */
class [[eosio::contract("token")]] token : public contract {
   public:
      using contract::contract;

      /**
       * Allows `issuer` account to create a token in supply of `maximum_supply`. If validation is successful a new entry in statstable for token symbol scope gets created.
       *
       * @param issuer - the account that creates the token,
       * @param maximum_supply - the maximum supply set for the token created.
       *
       * @pre Token symbol has to be valid,
       * @pre Token symbol must not be already created,
       * @pre maximum_supply has to be smaller than the maximum supply allowed by the system: 1^62 - 1.
       * @pre Maximum supply must be positive;
       * @param issuer - the issuer of token,
       * @param maximum_supply - the maximum_supply of token,
       */
      [[eosio::action]]
      void create( const name&   issuer,
                   const asset&  maximum_supply );
      /**
       *  This action issues to `to` account a `quantity` of tokens.
       *
       * @param to - the account to issue tokens to, it must be the same as the issuer,
       * @param quantity - the amount of tokens to be issued,
       * @param memo - the memo string that accompanies the token issue transaction.
       */
      [[eosio::action]]
      void issue( const name&   to,
                  const asset&  quantity,
                  const string& memo );

      /**
       * Allows `from` account to transfer to `to` account the `quantity` tokens.
       * One account is debited and the other is credited with quantity tokens.
       *
       * @param from - transfer from which account,
       * @param to - transfer to which account,
       * @param quantity - the quantity of tokens to be transferred,
       * @param memo - the memo string to accompany the transaction.
       */
      [[eosio::action]]
      void transfer( const name&    from,
                     const name&    to,
                     const asset&   quantity,
                     const string&  memo );

       /**
       *  This action set exchanging time.
       *
       * @param time - the exchanging time,
       * @param sym - the symbol of currency.
       */
      [[eosio::action]]
      void setextime( uint64_t time,
                      const symbol& sym );

      /**
       * This action will freeze an account.
       *
       * @param acc - the account to be frozen,
       * @param time - freeze until this time.
       */
      [[eosio::action]]
      void freezeacc( const name&    acc,
                      uint64_t       time );

      /**
       * This action will unfreeze an account.
       *
       * @param acc - the account to be unfrozen,
       */
      [[eosio::action]]
      void unfreezeacc( const name& acc );

      /**
       * This action will add acc to accbig.
       *
       * @param user - which account add to accbig,
       * @param desc - the desc.
       */
      [[eosio::action]]
      void addaccbig( const name&   user,
                      const string& desc );

      /**
       * This action will remove acc from accbig.
       *
       * @param user - which account,
       */
      [[eosio::action]]
      void rmvaccbig( const name& user );

      /**
       * This action will add rule for lock.
       *
       * @param lockruleid - id of the lock rule,
       * @param times - lock times,
       * @param pcts - lock percentages,
       * @param base - lock percentage's denominator,
       * @param isabsolute - is absolute time
       * @param desc - the desc.
       */

      [[eosio::action]]
      void addrule( uint64_t                 lockruleid,
                    const vector<uint64_t>&  times,
                    const vector<uint8_t>&   pcts, // lock percentage's numerator
                    uint8_t                  base, // lock percentage's denominator
                    bool                     isabsolute,
                    const string&            desc);
                  
      /**
       * This action will add rule for lock.
       *
       * @param lockruleid - id of the lock rule,
       * @param times - lock times,
       * @param pcts - lock percentages,
       * @param base - lock percentage's denominator,
       * @param isabsolute - is absolute time
       * @param desc - the desc.
       */
      
      /**
       * This action will transfer as locked asset.
       *
       * @param lockruleid - which account,
       * @param from - transfer from which account,
       * @param to - transfer to which account,
       * @param quantity - quantity,
       * @param memo - the memo.
       */
      [[eosio::action]]
      void locktransfer( uint64_t      lockruleid,
                         const name&   from,
                         const name&   to,
                         const asset&  quantity,
                         const string& memo );
      
      /**
       * This action is system transfer.
       *
       * @param from - transfer from which account,
       * @param to - transfer to which account,
       * @param quantity - quantity,
       * @param memo - the memo.
       */
      [[eosio::action]]
      void systransfer( const name&   from,
                        const name&   to,
                        const asset&  quantity,
                        const string& memo );

      static asset get_supply( const name& token_contract_account, const symbol_code& sym_code )
      {
         stats statstable( token_contract_account, sym_code.raw() );
         const auto& st = statstable.get( sym_code.raw() );
         return st.supply;
      }

      static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
      {
         accounts accountstable( token_contract_account, owner.value );
         const auto& ac = accountstable.get( sym_code.raw() );
         return ac.balance;
      }

      static bool is_frozen( const name& token_contract_account, const name&  user )
      {
         bool isFrozen = false;
         accfrozens _accfrozens( token_contract_account, token_contract_account.value );
         auto it = _accfrozens.find( user.value );
         if( it != _accfrozens.end() ) {
            uint64_t current_time = current_time_point().sec_since_epoch(); //seconds
            if( current_time < it->time ) {
               isFrozen = true;
            }
         }
         return isFrozen;
      }


      using create_action = eosio::action_wrapper<"create"_n, &token::create>;
      using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
      using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
      using setextime_action = eosio::action_wrapper<"setextime"_n, &token::setextime>;
      using freezeacc_action = eosio::action_wrapper<"freezeacc"_n, &token::freezeacc>;
      using unfreezeacc_action = eosio::action_wrapper<"unfreezeacc"_n, &token::unfreezeacc>;
      using addaccbig_action = eosio::action_wrapper<"addaccbig"_n, &token::addaccbig>;
      using rmvaccbig_action = eosio::action_wrapper<"rmvaccbig"_n, &token::rmvaccbig>;
      using addrule_action = eosio::action_wrapper<"addrule"_n, &token::addrule>;
      using locktransfer_action = eosio::action_wrapper<"locktransfer"_n, &token::locktransfer>;

   private:
      struct [[eosio::table]] account {
         asset    balance;

         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };

      struct [[eosio::table]] currency_stats {
         asset    supply;
         asset    max_supply;
         name     issuer;
         uint64_t extime = 0; // exchanging time

         uint64_t primary_key()const { return supply.symbol.code().raw(); }
      };

      struct [[eosio::table]] accfrozen {
         name     user;
         uint64_t time; // frozen deadline

         uint64_t primary_key()const { return user.value; }
      };

      struct [[eosio::table]] accbig {
         name       user;
         string     desc;

         uint64_t   primary_key()const { return user.value; }
      };

      struct [[eosio::table]] lockrule {
         uint64_t                lockruleid;
         vector<uint64_t>        times;
         vector<uint8_t>         pcts; // lock percentage's numerator
         uint8_t                 base; // lock percentage's denominator
         string                  desc;
         bool                    isabsolute;
         uint64_t                primary_key()const { return lockruleid; }
      };

      struct [[eosio::table]] acclock {
         uint64_t        lockruleid;  
         asset           quantity;
         name            user;
         name            from;
         uint64_t        time;

         uint64_t        primary_key()const { return lockruleid; }
      };

      typedef multi_index< "accounts"_n, account > accounts;
      typedef multi_index< "stat"_n, currency_stats > stats;
      typedef multi_index< "accfrozen"_n, accfrozen> accfrozens;
      typedef multi_index< "accbig"_n, accbig> accbigs;
      typedef multi_index< "lockrule"_n, lockrule> lockrules;
      typedef multi_index< "acclock"_n, acclock> acclocks;

      void sub_balance( const name& owner, const asset& value, bool is_force );
      void add_balance( const name& owner, const asset& value, const name& ram_payer );
      asset get_lock_asset( const name& user, const symbol& sym );

      bool check_frozen( const name&  user );
};
