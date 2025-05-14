import cv2
import numpy as np
import requests
from urllib.request import urlopen
import time

ESP32_CAM_URL = "http://192.168.40.205/capture"
  # Change to your ESP32-CAM IP
ESP32_SERVER_URL = "http://192.168.40.180/control"
  # Change to your Traffic Controller IP

qr_detector = cv2.QRCodeDetector()

# Load stored QR code
stored_image = cv2.imread("qrcode.png")
stored_data, _, _ = qr_detector.detectAndDecode(stored_image)

if not stored_data:
    print("Error: No QR code found in stored image.")
    exit()

print(f"Stored QR Code Data: {stored_data}")

while True:
    try:
        resp = urlopen(ESP32_CAM_URL)
        img_array = np.asarray(bytearray(resp.read()), dtype=np.uint8)
        frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    except:
        print("Failed to capture image. Check ESP32-CAM connection.")
        time.sleep(1)
        continue

    data, bbox, _ = qr_detector.detectAndDecode(frame)

    if data:
        print(f"QR Code detected: {data}")
        if data == stored_data:
            print("✅ QR Code MATCHED! Sending request to ESP32...")
            requests.get(f"{ESP32_SERVER_URL}?light=green")
            time.sleep(5)
            requests.get(f"{ESP32_SERVER_URL}?light=normal")
        else:
            print("❌ QR Code NOT MATCHED!")

    if bbox is not None:
        for i in range(len(bbox[0])):
            pt1 = tuple(map(int, bbox[0][i]))
            pt2 = tuple(map(int, bbox[0][(i + 1) % len(bbox[0])]))
            cv2.line(frame, pt1, pt2, (0, 255, 0), 2)
    cv2.imshow("QR Scanner", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()
