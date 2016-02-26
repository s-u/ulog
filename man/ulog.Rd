\name{ulog}
\alias{ulog}
\alias{ulog.init}
\title{
 System logging functions
}
\description{
 \code{ulog} sends output to a system log or ulog daemon.

 \code{ulog.init} defines where all logging will be directed to.
}
\usage{
ulog(...)
ulog.init(path = NULL, application = NULL)
}
\arguments{
  \item{path}{string, path to the unix socked of the logging daemon or
  specification of the form either "udp://host[:port]" or
  "tcp://host[:port]" for a remote connection. If \code{NULL} is
  passed the path setting is not changed.
  }
  \item{application}{string, name of the application that will be
  reported to the system or \code{NULL} to not change that setting.}
  \item{...}{any content to send to the log service - it is used as
  \code{paste(..., sep="", collapse="\n")}}
}
\details{
  \code{ulog} provides a way to perform logging without cluttering the
  console or stdout/stderr. It also allows multi-process and parallel
  logging as each log message is transmitted independently and
  en-bloc. Also it allow multi-user logging with access control.

  Although any syslog damon can be used, a minimalistic implementation
  of the daemon is included in the sources in \code{src/ulogd}.

  Note that all logging is silent and will not fail even if the
  receiving side doesnt' exist. This allows unconditional use of
  \code{ulog()}.

  This package has been forked from Rserve which has used ulog
  internally.
}
\value{
  \code{ulog} returns the logged string invisibly

  \code{ulog.init} returns the current logging path, thus
  \code{ulog.init()} can be used to query the current setting without
  changing anything.
} 
%\references{
%}
\author{
 Simon Urbanek
}
%\seealso{
%}
\examples{
ulog.init("/var/run/syslogd", "R")
ulog("a message from R")
}
\keyword{manip}
