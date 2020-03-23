#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup privileged_c Privileged C API
 * @ingroup c_api
 * @brief Defines %C Privileged API
 */

/**
 * Proposes a schedule change
 *
 * @note Once the block that contains the proposal becomes irreversible, the schedule is promoted to "pending" automatically. Once the block that promotes the schedule is irreversible, the schedule will become "active"
 * @param producer_data - packed data of produce_keys in the appropriate producer schedule order
 * @param producer_data_size - size of the data buffer
 *
 * @return -1 if proposing a new producer schedule was unsuccessful, otherwise returns the version of the new proposed schedule
 */
__attribute__((eosio_wasm_import))
int64_t set_proposed_producers( char *producer_data, uint32_t producer_data_size );

/**
 * Check if an account is privileged
 *
 * @param account - name of the account to be checked
 * @return true if the account is privileged
 * @return false if the account is not privileged
 */
__attribute__((eosio_wasm_import))
bool is_privileged( capi_name account );

/**
 * Set the blockchain parameters
 *
 * @param data - pointer to blockchain parameters packed as bytes
 * @param datalen - size of the packed blockchain parameters
 * @pre `data` is a valid pointer to a range of memory at least `datalen` bytes long that contains packed blockchain params data
 */
__attribute__((eosio_wasm_import))
void set_blockchain_parameters_packed( char* data, uint32_t datalen );

/**
 * Retrieve the blolckchain parameters
 *
 * @param data - output buffer of the blockchain parameters, only retrieved if sufficent size to hold packed data.
 * @param datalen - size of the data buffer, 0 to report required size.
 * @return size of the blockchain parameters
 * @pre `data` is a valid pointer to a range of memory at least `datalen` bytes long
 * @post `data` is filled with packed blockchain parameters
 */
__attribute__((eosio_wasm_import))
uint32_t get_blockchain_parameters_packed( char* data, uint32_t datalen );

/**
 */
__attribute__((eosio_wasm_import))
void update_stake_proxied( uint64_t token_code_raw, capi_name account, int32_t force );

/**
 */
__attribute__((eosio_wasm_import))
void recall_stake_proxied( uint64_t token_code_raw, capi_name grantor_name, capi_name recipient_name, int32_t pct );

__attribute__((eosio_wasm_import))
void recall_stake_proxied_soft( uint64_t token_code_raw, capi_name grantor_name, capi_name recipient_name, int32_t pct );

__attribute__((eosio_wasm_import))
uint64_t get_used_resources_cost( capi_name account );

#ifdef __cplusplus
}
#endif
