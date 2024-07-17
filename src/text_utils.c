#include "nested_plugin.h"
#include "text.h"

bool msg_display_address_ui(ethQueryContractUI_t *msg, uint8_t *address) {
    msg->msg[0] = '0';
    msg->msg[1] = 'x';
    return getEthAddressStringFromBinary((uint8_t *) address,
                                         (char *) msg->msg + 2,
                                         0);
}

/**
 * @param token choose token1 or token2
 */
bool msg_ticker_or_address(ethQueryContractUI_t *msg, context_t *context, int token) {
    bool ret = false;

    PRINTF("[msg_ticker_or_address] token: %d\n", token);
    PRINTF("[msg_ticker_or_address] TOKEN1_FOUND: %d\n", context->booleans & TOKEN1_FOUND);
    PRINTF("[msg_ticker_or_address] TOKEN2_FOUND: %d\n", context->booleans & TOKEN2_FOUND);
    PRINTF("[msg_ticker_or_address] number_of_tokens: %d\n", context->number_of_tokens);
    if (token == 1)  // Token1
    {
        if (context->booleans & TOKEN1_FOUND) {
            snprintf(msg->msg, msg->msgLength, "%s", context->token1_ticker);
            ret = true;
        } else {
            ret = msg_display_address_ui(msg, context->token1_address);
        }
    } else if (token == 2)  // Token2
    {
        if (context->booleans & TOKEN2_FOUND) {
            snprintf(msg->msg, msg->msgLength, "%s", context->token2_ticker);
            ret = true;
        } else if (context->ui_selector == WITHDRAW || context->ui_selector == PROPO_WITHDRAWAL ||
                   context->number_of_tokens == 1) {
            ret = msg_display_address_ui(msg, context->token2_address);
        }
    } else {
        snprintf(msg->msg, msg->msgLength, "ERROR");
    }
    return ret;
}

bool msg_2tickers_ui(ethQueryContractUI_t *msg, context_t *context) {
    snprintf(msg->msg, msg->msgLength, "%s and %s", context->token1_ticker, context->token2_ticker);
    return true;
}

/**
 * @param token choose token1 or token2
 */
bool msg_number_of_tokens(ethQueryContractUI_t *msg, context_t *context, int token) {
    if (context->number_of_tokens == 1) {
        if (token == 1 && context->booleans & TOKEN1_FOUND) {
            snprintf(msg->msg, msg->msgLength, "%s", context->token1_ticker);
        } else if (token == 2 && context->booleans & TOKEN2_FOUND) {
            snprintf(msg->msg, msg->msgLength, "%s", context->token2_ticker);
        } else if (!(context->booleans & TOKEN1_FOUND) || !(context->booleans & TOKEN2_FOUND)) {
            snprintf(msg->msg, msg->msgLength, "%d token", context->number_of_tokens);
        } else {
            snprintf(msg->msg, msg->msgLength, "ERROR");
            return false;
        }
    } else {
        snprintf(msg->msg, msg->msgLength, "%d tokens", context->number_of_tokens);
    }
    return true;
}

bool msg_amount_or_address_ui(ethQueryContractUI_t *msg,
                              context_t *context)  // Use for token1 only.
{
    bool ret = false;

    if (context->booleans & TOKEN1_FOUND) {
        ret = amountToString(context->token1_amount,
                             sizeof(context->token1_amount),
                             context->token1_decimals,
                             context->token1_ticker,
                             msg->msg,
                             msg->msgLength);
    } else {
        ret = msg_display_address_ui(msg, context->token1_address);
    }
    return ret;
}
