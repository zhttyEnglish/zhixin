import os
import cv2
import numpy as np
from PIL import Image

# write rgb data
"""
img = Image.open('/home/dl/workspace/项目交付/智芯/SCA200X_SDK_V1.5.3_GLIBC/mpp/sample/lenovo_alg/aqm2.jpeg')
img = img.resize((640, 640))
with open('rgb_img/aqm2.rgb', 'wb') as f:
    f.write(img.tobytes())
"""
img = Image.open('/home/dl/workspace/项目交付/智芯/SCA200X_SDK_V1.5.3_GLIBC/mpp/sample/lenovo_alg/person.jpeg')
img = img.resize((1920, 1080))
with open('person1920.rgb', 'wb') as f:
    f.write(img.tobytes())


img.save('person1920.jpg')







# read rgb data
# with open('rgb_img/rsrd.rgb', 'rb') as f:
#     data = f.read()

# print(len(data))

# data = [int(x) for x in data]
# data = np.array(data).reshape((640, 640, 3)).astype(np.uint8)
# # data = data.transpose((1, 2, 0))
# # cv2.imwrite('rgb.jpg', data)
# img = Image.fromarray(data)
# img.save('rgb.jpg')
