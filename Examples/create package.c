/*
    1) Create and initialize Sheller
*/
#define USEFULL_DATA_LENGTH 8
sheller_t shell;
sheller_init(&shell, 0x23, USEFULL_DATA_LENGTH, 128);

/* 
    2) Declaration  the buffer with the data you want to send.
*/
uint8_t data[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

/*
    3) Declaration the buffer for storing wrappering data
    * Size of wrappers message not dependent on user data length. 
*/
uint8_t wrappered_data[USEFULL_DATA_LENGTH + SHELLER_SERVICE_BYTES_COUNT] = {0};

/*
    4) Call sheller_wrap
*/
uint8_t shell_result = sheller_wrap(&shell, data, 8, data_wrapered);
if (shell_result == SHELLER_ERROR) {
    //Handle error. Check the shell pointer, the data pointer, the length of data, the pointer of data_wrapered
} else {
    //Send data_wrapered buff via channel
}