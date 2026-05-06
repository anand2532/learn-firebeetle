# Current Sensor Data Service

This directory contains the EC2-side receiver and a `systemd` service so ingestion runs independently.

## Files

- `ec2_data_receiver.py`: HTTP server on port `5000`
- `current-sensor-data.service`: `systemd` service unit

## EC2 Setup

Copy this directory to EC2:

```bash
scp -i ./dev_key.pem -r ./current-sensor-data ubuntu@15.207.18.221:~/
```

SSH and install service:

```bash
ssh -i ./dev_key.pem ubuntu@15.207.18.221
sudo cp /home/ubuntu/current-sensor-data/current-sensor-data.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable current-sensor-data.service
sudo systemctl start current-sensor-data.service
```

## Verify

```bash
sudo systemctl status current-sensor-data.service
curl http://127.0.0.1:5000/health
tail -f /home/ubuntu/current-sensor-data/incoming_data/sensor_readings.jsonl
```
