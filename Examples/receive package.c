/*
    1) Create and initialize Sheller
*/
sheller_t shell;
sheller_init(&shell);

/*
    2) Declaration the buffer for storing received data
    * The length of  data is static and defined by SHELLER_USEFULL_DATA_LENGTH = 8
*/
uint8_t received_data[SHELLER_USEFULL_DATA_LENGTH] = {0};

/*
    3) Push received bytes to Sheller via function sheller_push
*/
sheller_push(&shell, received_byte);

/*
    4) In main loop call as often as posiible sheller_read
*/
uint8_t read_result = sheller_read(&shell, received_data);
if (read_result == SHELLER_OK) {
    // Package received. The usefull data storing in received_data buff
}