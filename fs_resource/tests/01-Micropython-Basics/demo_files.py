import os

mount_points = os.listdir("/")
for fs in mount_points:
    print("------------")
    print(" dir:", fs)
    os.listdir("/"+fs)
