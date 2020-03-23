#pragma once
#include "producer_schedule.hpp"
#include "../../core/eosio/crypto.hpp"
#include "../../core/eosio/name.hpp"
#include "../../core/eosio/symbol.hpp"
#include "../../core/eosio/serialize.hpp"

namespace eosio {

   namespace internal_use_do_not_use {
      extern "C" {
         __attribute__((eosio_wasm_import))
         bool is_privileged( uint64_t account );

        __attribute__((eosio_wasm_import))
        void set_blockchain_parameters_packed( char* data, uint32_t datalen );

        __attribute__((eosio_wasm_import))
        uint32_t get_blockchain_parameters_packed( char* data, uint32_t datalen );

        __attribute((eosio_wasm_import))
        int64_t set_proposed_producers( char*, uint32_t );

        __attribute((eosio_wasm_import))
        void update_stake_proxied( uint64_t token_code_raw, uint64_t account, int32_t force );

        __attribute((eosio_wasm_import))
        void recall_stake_proxied( uint64_t token_code_raw, uint64_t grantor_name, uint64_t recipient_name, int32_t pct );
        
        __attribute((eosio_wasm_import))
        void recall_stake_proxied_soft( uint64_t token_code_raw, uint64_t grantor_name, uint64_t recipient_name, int32_t pct );
        
        __attribute((eosio_wasm_import))
        uint64_t get_used_resources_cost(  uint64_t account );
      }
   }

  /**
   *  @defgroup privileged Privileged
   *  @ingroup contracts
   *  @brief Defines C++ Privileged API
   */

   /**
    *  Tunable blockchain configuration that can be changed via consensus
    *  @ingroup privileged
    */
   struct blockchain_parameters {
      uint32_t   base_per_transaction_net_usage;      ///< the base amount of net usage billed for a transaction to cover incidentals
      uint32_t   context_free_discount_net_usage_num; ///< the numerator for the discount on net usage of context-free data
      uint32_t   context_free_discount_net_usage_den; ///< the denominator for the discount on net usage of context-free data

      uint32_t   min_transaction_cpu_usage;           ///< the minimum billable cpu usage (in microseconds) that the chain requires

      uint64_t   min_transaction_ram_usage;           ///< the minimum billable ram usage (in bytes) that the chain requires

      uint32_t   max_transaction_lifetime;            ///< the maximum number of seconds that an input transaction's expiration can be ahead of the time of the block in which it is first included
      uint32_t   deferred_trx_expiration_window;      ///< the number of seconds after the time a deferred transaction can first execute until it expires
      uint32_t   max_transaction_delay;               ///< the maximum number of seconds that can be imposed as a delay requirement by authorization checks
      uint32_t   max_inline_action_size;              ///< maximum allowed size (in bytes) of an inline action
      uint16_t   max_inline_action_depth;             ///< recursion depth limit on sending inline actions
      uint16_t   max_authority_depth;                 ///< recursion depth limit for checking if an authority is satisfied

      uint64_t   ram_size;
      uint64_t   reserved_ram_size;

      std::vector<uint64_t> max_block_usage;
      std::vector<uint64_t> max_transaction_usage;

      std::vector<uint64_t> target_virtual_limits;
      std::vector<uint64_t> min_virtual_limits;
      std::vector<uint64_t> max_virtual_limits;
      std::vector<uint32_t> usage_windows;

      std::vector<uint16_t> virtual_limit_decrease_pct;
      std::vector<uint16_t> virtual_limit_increase_pct;

      std::vector<uint32_t> account_usage_windows;

      EOSLIB_SERIALIZE( blockchain_parameters,
           (base_per_transaction_net_usage)
           (context_free_discount_net_usage_num)(context_free_discount_net_usage_den)

           (min_transaction_cpu_usage)
           (min_transaction_ram_usage)

           (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)
           (max_inline_action_size)(max_inline_action_depth)(max_authority_depth)

           (ram_size)(reserved_ram_size)
           (max_block_usage)(max_transaction_usage)

           (target_virtual_limits)(min_virtual_limits)(max_virtual_limits)(usage_windows)
           (virtual_limit_decrease_pct)(virtual_limit_increase_pct)(account_usage_windows)
      )
   };

   /**
    *  Set the blockchain parameters
    *
    *  @ingroup privileged
    *  @param params - New blockchain parameters to set
    */
   void set_blockchain_parameters(const eosio::blockchain_parameters& params);

   /**
    *  Retrieve the blolckchain parameters
    *
    *  @ingroup privileged
    *  @param params - It will be replaced with the retrieved blockchain params
    */
   void get_blockchain_parameters(eosio::blockchain_parameters& params);

   /**
    *  Proposes a schedule change
    *
    *  @ingroup privileged
    *  @note Once the block that contains the proposal becomes irreversible, the schedule is promoted to "pending" automatically. Once the block that promotes the schedule is irreversible, the schedule will become "active"
    *  @param prods - vector of producer keys
    *
    *  @return an optional value of the version of the new proposed schedule if successful
    */
   std::optional<uint64_t> set_proposed_producers( const std::vector<producer_key>& prods );

   /**
    *  Check if an account is privileged
    *
    *  @ingroup privileged
    *  @param account - name of the account to be checked
    *  @return true if the account is privileged
    *  @return false if the account is not privileged
    */
   inline bool is_privileged( name account ) {
      return internal_use_do_not_use::is_privileged( account.value );
   }

   inline void update_stake_proxied( symbol_code token_code, name account, bool force ) {
      return internal_use_do_not_use::update_stake_proxied(token_code.raw(), account.value, force);
   }

   inline void recall_stake_proxied( symbol_code token_code, name grantor_name, name recipient_name, int32_t pct, bool zero_amount_allowed = false) {
      return zero_amount_allowed ?
         internal_use_do_not_use::recall_stake_proxied_soft(token_code.raw(), grantor_name.value, recipient_name.value, pct) :
         internal_use_do_not_use::recall_stake_proxied     (token_code.raw(), grantor_name.value, recipient_name.value, pct);
   }
   
   inline uint64_t get_used_resources_cost( name account ) {
      return internal_use_do_not_use::get_used_resources_cost(account.value);
   }
}
