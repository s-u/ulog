ulog <- function(...) invisible(.Call(C_ulog, paste(..., collapse = "\n", sep = "")))

ulog.init <- function(path = NULL, application = NULL) .Call(C_ulog_init, path, application)
