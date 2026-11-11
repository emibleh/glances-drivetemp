# glances-drivetemp

## use in Docker compose
```
volumes:
      - /dev:/dev
    depends_on:
      - drivetemp

  drivetemp:
    image: emibleh/glances-drivetemp:1.1
    container_name: drivetemp
    restart: unless-stopped
    privileged: true
    volumes:
      - /dev:/dev
```
