#include "nested_plugin.h"

// Called once to init.
void handle_init_contract(ethPluginInitContract_t *msg) {
    PRINTF("IN handle_init_contract\n");

    // Make sure we are running a compatible version.
    if (msg->interfaceVersion != ETH_PLUGIN_INTERFACE_VERSION_LATEST) {
        // If not the case, return the `UNAVAILABLE` status.
        msg->result = ETH_PLUGIN_RESULT_UNAVAILABLE;
        return;
    }

    // Print size of plugin's context.
    PRINTF("context size: %d / %d\n", sizeof(context_t), msg->pluginContextLength);

    // Double check that the `context_t` struct is not bigger than the maximum
    // size (defined by `msg->pluginContextLength`).
    if (msg->pluginContextLength < sizeof(context_t)) {
        PRINTF("Plugin parameters structure is bigger than allowed size\n");
        msg->result = ETH_PLUGIN_RESULT_ERROR;
        return;
    }

    context_t *context = (context_t *) msg->pluginContext;

    // Initialize the context (to 0).
    memset(context, 0, sizeof(*context));

    // Find tx selector
    size_t index;
    if (!find_selector(U4BE(msg->selector, 0), NESTED_SELECTORS, NUM_SELECTORS, &index)) {
        PRINTF("can't find selector\n");
        msg->result = ETH_PLUGIN_RESULT_UNAVAILABLE;
        return;
    }
    context->selectorIndex = index;

    // Set `next_param` to be the first field we expect to parse.
    switch (context->selectorIndex) {
        case CREATE:
            PRINTF("IN CREATE\n");
            context->next_param = CREATE__TOKEN_ID;
            break;
        case PROCESS_INPUT_ORDERS:
            PRINTF("IN PROCESS_INPUT_ORDERS\n");
            context->next_param = CREATE__TOKEN_ID;
            break;
        case PROCESS_OUTPUT_ORDERS:
            PRINTF("IN PROCESS_OUTPUT_ORDERS\n");
            context->next_param = CREATE__TOKEN_ID;
            break;
        case DESTROY:
            PRINTF("IN DESTROY\n");
            context->next_param = DESTROY__TOKEN_ID;
            break;
        case RELEASE_TOKENS:
            PRINTF("IN RELEASE TOKENS\n");
            context->next_param = RELEASE__OFFSET_TOKENS;
            break;
        case TRANSFER_FROM:
            PRINTF("IN TRANSFER TRANSFER_FROM__FROM\n");
            context->next_param = TRANSFER_FROM__FROM;
            break;
        default:
            PRINTF("Missing selectorIndex: %d\n", context->selectorIndex);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            return;
    }

    // Return valid status.
    msg->result = ETH_PLUGIN_RESULT_OK;
}
