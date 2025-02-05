#include "nested_plugin.h"

/*
   Check token id, set IS_COPY is token id is not '000...'
   */
static void check_token_id(ethPluginProvideParameter_t *msg, context_t *context) {
    for (uint8_t i = 0; i < PARAMETER_LENGTH; i++) {
        if (msg->parameter[i] != 0) {
            context->booleans |= IS_COPY;
            break;
        }
    }
}

/*
   parse both create, processInputOrder and processOutputOrder.
   */
static void handle_create(ethPluginProvideParameter_t *msg, context_t *context) {
    // Switch to current struct parsing function.
    if (context->on_struct) {
        if (context->on_struct == S_BATCHED_INPUT_ORDERS)
            parse_batched_input_orders(msg, context);
        else if (context->on_struct == S_BATCHED_OUTPUT_ORDERS)
            parse_batched_output_orders(msg, context);
        else if (context->on_struct == S_ORDER)
            parse_order(msg, context);
        else {
            PRINTF("handle_create on_struct ERROR\n");
            msg->result = ETH_PLUGIN_RESULT_ERROR;
        }
        return;
    }
    switch ((create_parameter) context->next_param) {
        case CREATE__TOKEN_ID:
            PRINTF("CREATE__TOKEN_ID\n");
            // Check if it's copy or create
            if (context->selectorIndex == CREATE) check_token_id(msg, context);
            // We could get 'tokenId' here
            context->next_param = (create_parameter) CREATE__OFFSET_BIO;
            break;
        case CREATE__OFFSET_BIO:
            PRINTF("CREATE__OFFSET_BIO\n");
            // No need to copy the offset here, because it's always the next parameter
            context->next_param = (create_parameter) CREATE__LEN_BIO;
            break;
        case CREATE__LEN_BIO:
            PRINTF("CREATE__LEN_BIO\n");
            // copy BIO/BOO[] len to get last BIO/BOO offset.
            if (!copy_number(msg->parameter, &context->current_length) ||
                !context->current_length) {  // if BIO/BOO[] have no items.
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            // reset current_tuple_offset to current tx offset.
            context->current_tuple_offset = msg->parameterOffset;
            context->next_param = (create_parameter) CREATE__OFFSET_ARRAY_BIO;
            break;
        case CREATE__OFFSET_ARRAY_BIO:
            if (context->current_length) context->current_length--;
            PRINTF("CREATE__OFFSET_ARRAY_BIO, index: %d\n", context->current_length);
            // If we are on last offset. skip otherwise.
            if (context->current_length == 0) {
                // copy last_batch_offset only once.
                if (!context->last_batch_offset) {
                    uint32_t buf = 0;
                    // Copy offset
                    if (!copy_number(msg->parameter, &buf)) {
                        msg->result = ETH_PLUGIN_RESULT_ERROR;
                        return;
                    }
                    // Add current_tuple_offset to get the real target offset.
                    if (!add_numbers(&context->last_batch_offset,
                                     buf + context->current_tuple_offset)) {
                        msg->result = ETH_PLUGIN_RESULT_ERROR;
                        return;
                    }
                    PRINTF("context->last_batch_offset: %d\n", context->last_batch_offset);
                }
                PRINTF("is parameterOffset: %d == last_batch_offset: %d ?\n",
                       msg->parameterOffset,
                       context->last_batch_offset);
                // Switch to according struct's parsing method. only on last BIO/BOO
                if (msg->parameterOffset == context->last_batch_offset) {
                    switch (context->selectorIndex) {
                        case CREATE:
                        case PROCESS_INPUT_ORDERS:
                            context->on_struct = (on_struct) S_BATCHED_INPUT_ORDERS;
                            context->next_param = (batch_input_orders) BIO__INPUTTOKEN;
                            break;
                        case PROCESS_OUTPUT_ORDERS:
                            context->on_struct = (on_struct) S_BATCHED_OUTPUT_ORDERS;
                            context->next_param = (batch_output_orders) BOO__OUTPUTTOKEN;
                            break;
                        default:
                            PRINTF("Selector Index not supported: %d\n", context->selectorIndex);
                            msg->result = ETH_PLUGIN_RESULT_ERROR;
                            break;
                    }
                }
            }
            break;
        default:
            PRINTF("Param not supported: %d\n", context->next_param);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

static void handle_destroy(ethPluginProvideParameter_t *msg, context_t *context) {
    // Switch to current struct parsing function.
    if (context->on_struct == S_ORDER) {
        parse_order(msg, context);
        return;
    }
    switch ((destroy_parameter) context->next_param) {
        case DESTROY__TOKEN_ID:
            PRINTF("DESTROY TOKEN ID\n");
            context->next_param = (destroy_parameter) DESTROY__BUY_TOKEN;
            break;
        case DESTROY__BUY_TOKEN:
            PRINTF("DESTROY BUY TOKEN\n");
            copy_address(context->token1_address, msg->parameter, ADDRESS_LENGTH);
            PRINTF("Copied buyToken to token1_address: %.*H\n",
                   ADDRESS_LENGTH,
                   context->token1_address);
            context->next_param = (destroy_parameter) DESTROY__OFFSET_ORDERS;
            break;
        case DESTROY__OFFSET_ORDERS:
            PRINTF("DESTROY OFFSET ORDERS\n");
            // No need to copy the offset here, because it's always the next parameter
            context->next_param = (destroy_parameter) DESTROY__LEN_ORDERS;
            break;
        case DESTROY__LEN_ORDERS:
            PRINTF("DESTROY LEN ORDERS\n");
            if (!copy_number(msg->parameter, &context->number_of_tokens)) {
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            PRINTF("number_of_tokens: %d\n", context->number_of_tokens);
            context->next_param = (destroy_parameter) DESTROY__ORDERS;
            break;
        case DESTROY__ORDERS:
            PRINTF("DESTROY ORDERS");
            // Skip order's parsing if it is empty
            if (context->number_of_tokens == 0) return;
            // Switch to order's parsing
            context->on_struct = (on_struct) S_ORDER;
            context->next_param = (order) ORDER__OPERATOR;
            break;
        default:
            PRINTF("Param not supported: %d\n", context->next_param);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

static void handle_release_tokens(ethPluginProvideParameter_t *msg, context_t *context) {
    switch ((release_tokens_parameter) context->next_param) {
        case RELEASE__OFFSET_TOKENS:
            PRINTF("RELEASE__OFFSET_TOKENS\n");
            // No need to copy the offset here, because it's always the next parameter
            context->next_param = (release_tokens_parameter) RELEASE__LEN_TOKENS;
            break;
        case RELEASE__LEN_TOKENS:
            PRINTF("RELEASE__LEN_TOKENS\n");
            if (!copy_number(msg->parameter, &context->number_of_tokens) ||
                !copy_number(msg->parameter, &context->current_length) ||
                !context->current_length) {  // if tokens list have no items.
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            PRINTF("number_of_tokens: %d\n", context->number_of_tokens);
            context->next_param = (release_tokens_parameter) RELEASE__ARRAY_TOKENS;
            break;
        case RELEASE__ARRAY_TOKENS:
            // is first array element
            if (context->number_of_tokens == context->current_length) {
                PRINTF("RELEASE copy first token address.\n");
                copy_address(context->token1_address, msg->parameter, ADDRESS_LENGTH);
                PRINTF("Copied to token1_address: %.*H\n", ADDRESS_LENGTH, context->token1_address);
            }
            if (context->current_length) context->current_length--;
            // is last array element && multiple tokens
            if (context->number_of_tokens > 1 && context->current_length == 0) {
                PRINTF("RELEASE copy last token address.\n");
                copy_address(context->token2_address, msg->parameter, ADDRESS_LENGTH);
                PRINTF("Copied to token2_address: %.*H\n", ADDRESS_LENGTH, context->token2_address);
            }
            PRINTF("RELEASE_TOKENS token index: %d\n", context->current_length);
            if (!context->current_length)
                context->next_param++;  // Set error once all tokens are parsed.
            break;
        default:
            PRINTF("Param not supported: %d\n", context->next_param);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

static void handle_transfer_from(ethPluginProvideParameter_t *msg, context_t *context) {
    switch ((transfer_from_parameter) context->next_param) {
        case TRANSFER_FROM__FROM:
            context->next_param = (transfer_from_parameter) TRANSFER_FROM__TO;
            break;
        case TRANSFER_FROM__TO:
            copy_address(context->token1_address, msg->parameter, ADDRESS_LENGTH);
            context->next_param = (transfer_from_parameter) TRANSFER_FROM__TOKEN_ID;
            break;
        case TRANSFER_FROM__TOKEN_ID:
            context->next_param++;  // Set error.
            break;
        default:
            PRINTF("Param not supported: %d\n", context->next_param);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

void handle_provide_parameter(ethPluginProvideParameter_t *msg) {
    context_t *context = (context_t *) msg->pluginContext;
    // We use `%.*H`: it's a utility function to print bytes. You first give
    // the number of bytes you wish to print (in this case, `PARAMETER_LENGTH`)
    // and then the address (here `msg->parameter`).
    PRINTF(
        "___\nplugin provide parameter: offset %d\nBytes: \033[0;31m %.*H "
        "\033[0m \n",
        msg->parameterOffset,
        PARAMETER_LENGTH,
        msg->parameter);

    msg->result = ETH_PLUGIN_RESULT_OK;

    switch (context->selectorIndex) {
            // create, processInputOrders and processOutputOrders have similar
            // prototype, so we use the same parsing method.
        case CREATE:
        case PROCESS_INPUT_ORDERS:
        case PROCESS_OUTPUT_ORDERS:
            handle_create(msg, context);
            break;
        case DESTROY:
            handle_destroy(msg, context);
            break;
        case RELEASE_TOKENS:
            handle_release_tokens(msg, context);
            break;
        case TRANSFER_FROM:
            handle_transfer_from(msg, context);
            break;
        default:
            PRINTF("Selector Index not supported: %d\n", context->selectorIndex);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}
