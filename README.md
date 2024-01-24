# clog.h  
A single header C logging library.  
---
### Example
```C
#include "clog.h"

int main(void) {
    // default format (%Y-%m-%d %H:%M:%S)
    Clog* ci = clog_default();

    // enable colored output
    clog_color_enable(ci);

    // check if managed to open file
    if (!clog_output_to_file(ci, "log.txt", CLOG_FILE_WRITE)) {
        perror("failed to open log to log.txt");
    }

    // assert or log
    clog_assert(ci, 1 == 1);
    //clog_assert(ci, 1 == 2); // will print something and then exit

    clog_info(ci, "here is an info message");
    clog_warn(ci, "This is a warning!!!");
    clog_error(ci, "An error has occurred.");

    clog_newline(ci); // print a newline to stdout and the opened log (if opened)

    int x = 1234;

    clog_warn_fmt(ci, "This is a formatted warning! x = %d", x);

    // get the appropiate callback from loglevel
    clog_from_level(CLOG_ERROR)(ci, "Hello from the constructed callback!");

    // changing
    clog_set_minimum_level(ci, CLOG_WARNING);
    clog_info(ci, "This will not be shown!");
    clog_set_minimum_level(ci, CLOG_INFO);

    clog_warn_mult(ci, "Here", "Are", "Multiple", "Warning", "From", "1", "Call!");

    clog_color_disable(ci);
    clog_info(ci, "Colorless message");
    clog_color_enable(ci);

    // always remember to free!
    clog_free(ci);

    return 0;
}

```