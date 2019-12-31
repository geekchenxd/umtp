#include <stdint.h>
#include "service.h"

struct fttp_app {
	uint8_t pdu_type;
	uint8_t service_choice;
	uint8_t session_id;
	struct service_data *data;
};
