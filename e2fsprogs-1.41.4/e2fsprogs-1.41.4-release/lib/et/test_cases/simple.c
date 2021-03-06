/*
 * simple.c:
 * This file is automatically generated; please do not edit it.
 */

#include <stdlib.h>

static const char * const text[] = {
				"Can't read ticket file",
				"Can't find ticket or TGT",
				"TGT expired",
				"Can't decode authenticator",
				"Ticket expired",
				"Repeated request",
				"The ticket isn't for us",
				"Request is inconsistent",
				"Delta-T too big",
				"Incorrect net address",
				"Protocol version mismatch",
				"Invalid message type",
				"Message stream modified",
				"Message out of order",
				"Unauthorized request",
				"Current password is null",
				"Incorrect current password",
				"Protocol error",
				"Error returned by KDC",
				"Null ticket returned by KDC",
				"Retry count exceeded",
				"Can't send request",
    0
};

struct error_table {
    char const * const * msgs;
    long base;
    int n_msgs;
};
struct et_list {
    struct et_list *next;
    const struct error_table * table;
};
extern struct et_list *_et_list;

const struct error_table et_krb_error_table = { text, 39525376L, 22 };

static struct et_list link = { 0, 0 };

void initialize_krb_error_table_r(struct et_list **list);
void initialize_krb_error_table(void);

void initialize_krb_error_table(void) {
    initialize_krb_error_table_r(&_et_list);
}

/* For Heimdal compatibility */
void initialize_krb_error_table_r(struct et_list **list)
{
    struct et_list *et, **end;

    for (end = list, et = *list; et; end = &et->next, et = et->next)
        if (et->table->msgs == text)
            return;
    et = malloc(sizeof(struct et_list));
    if (et == 0) {
        if (!link.table)
            et = &link;
        else
            return;
    }
    et->table = &et_krb_error_table;
    et->next = 0;
    *end = et;
}
