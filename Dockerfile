FROM python:3.12-alpine

RUN apk add --no-cache smartmontools

COPY hddtemp_server.py /hddtemp_server.py

EXPOSE 7634

CMD ["python3", "/hddtemp_server.py"]
