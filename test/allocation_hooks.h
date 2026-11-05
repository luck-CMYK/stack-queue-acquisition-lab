#ifndef ALLOCATION_HOOKS_H
#define ALLOCATION_HOOKS_H
/* Force-include for production translation units in the audited test build.
 * Do not force-include for allocation_audit.c or the C runtime itself.
 */
#include <stdlib.h>
void *audit_malloc(size_t size);
void audit_free(void *pointer);
#define malloc audit_malloc
#define free audit_free
#endif
