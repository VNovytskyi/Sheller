/*
    1) Create and initialize Sheller
*/
sheller_t shell;
sheller_init(&shell, 0x23, 8, 128);

/* 
    2) Declaration  the buffer with the data you want to send.
    * The length of the data must be equal or less than SHELLER_USEFULL_DATA_LENGTH = 8 !
*/
uint8_t data[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

/*
    3) Declaration the buffer for storing wrappering data
    * Size of wrappers message not dependent on user data length. 
    * The length of wrapped data is static and defined by SHELLER_PACKAGE_LENGTH = 11
*/
uint8_t wrappered_data[SHELLER_PACKAGE_LENGTH] = {0};

/*
    4) Call sheller_wrap
*/
uint8_t shell_result = sheller_wrap(&shell, data, 8, data_wrapered);
if (shell_result == SHELLER_ERROR) {
    //Handle error. Check the shell pointer, the data pointer, the length of data, the pointer of data_wrapered
} else {
    //Send data_wrapered buff (11 bytes)  via channel
}