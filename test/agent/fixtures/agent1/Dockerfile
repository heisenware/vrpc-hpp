FROM alpine:3.14.2 as builder
RUN apk add --no-cache g++ boost-dev
COPY . /app
WORKDIR /app
RUN g++ -I. -pthread -D VRPC_DEBUG -o agent main.cpp

FROM alpine:3.14.2
RUN apk add --no-cache libstdc++ libgcc
COPY --from=builder /app/agent /app/
ENTRYPOINT [ "/app/agent" ]
