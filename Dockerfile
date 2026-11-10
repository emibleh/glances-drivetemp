# Build
FROM alpine:3.20 AS build
RUN apk add --no-cache g++ musl-dev make
WORKDIR /src
RUN g++ -O2 -static -o hddtemp_server hddtemp_server.cpp

# Runtime
FROM alpine:3.20
RUN apk add --no-cache smartmontools
COPY --from=build /src/hddtemp_server /hddtemp_server
EXPOSE 7634
CMD ["/hddtemp_server"]
