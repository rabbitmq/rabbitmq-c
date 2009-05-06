#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <unistd.h>
#include <assert.h>

int main(int argc, char const * const *argv) {
  amqp_table_entry_t entries[8] = { AMQP_TABLE_ENTRY_S("zebra", amqp_cstring_bytes("last")),
				    AMQP_TABLE_ENTRY_S("aardvark", amqp_cstring_bytes("first")),
				    AMQP_TABLE_ENTRY_S("middle", amqp_cstring_bytes("third")),
				    AMQP_TABLE_ENTRY_I("number", 1234),
				    AMQP_TABLE_ENTRY_D("decimal", AMQP_DECIMAL(2, 1234)),
				    AMQP_TABLE_ENTRY_T("time", (uint64_t) 1234123412341234LL),
				    AMQP_TABLE_ENTRY_S("beta", amqp_cstring_bytes("second")),
				    AMQP_TABLE_ENTRY_S("wombat", amqp_cstring_bytes("fourth")) };
  amqp_table_t table = { .num_entries = sizeof(entries) / sizeof(entries[0]),
			 .entries = &entries[0] };
  int i;

  qsort(table.entries, table.num_entries, sizeof(amqp_table_entry_t), &amqp_table_entry_cmp);

  for (i = 0; i < table.num_entries; i++) {
    amqp_table_entry_t *e = &table.entries[i];
    printf("%.*s -> %c (", (int) e->key.len, (char *) e->key.bytes, e->kind);
    switch (e->kind) {
      case 'S': printf("%.*s", (int) e->value.bytes.len, (char *) e->value.bytes.bytes); break;
      case 'I': printf("%d", e->value.i32); break;
      case 'D': printf("%d:::%u", e->value.decimal.decimals, e->value.decimal.value); break;
      case 'T': printf("%llu", e->value.u64); break;
      case 'F': printf("..."); break;
      default: printf("???"); break;
    }
    printf(")\n");
  }

  return 0;
}
