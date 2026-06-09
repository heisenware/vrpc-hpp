FROM alpine:3.14.2 as builder
RUN apk add --no-cache g++ linux-headers
COPY . /app
WORKDIR /app
RUN g++ main.cpp -I. -I./vrpc/boost -pthread -D VRPC_DEBUG -o agent

FROM alpine:3.14.2
RUN apk add --no-cache libstdc++ libgcc
COPY --from=builder /app/agent /app/
ENTRYPOINT [ "/app/agent" ]
