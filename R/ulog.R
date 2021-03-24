ulog <- function(...)
	invisible(.Call(C_ulog, paste(..., collapse = "\n", sep = "")))

ulog.init <- function(path = "udp://127.0.0.1:5140", application = NULL)
	.Call(C_ulog_init, path, application)

ulogd <- function(port=5140L) .C("C_ulogd",
				 as.integer(port))
