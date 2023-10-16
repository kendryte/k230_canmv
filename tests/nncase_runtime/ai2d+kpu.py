import nncase_runtime as nn
import ulab.numpy as np
import utime
import time
# init kpu and load kmodel
kpu = nn.kpu()
ai2d = nn.ai2d()
kpu.load_kmodel("/sdcard/app/tests/nncase_runtime/face_detection/face_detection.kmodel")

# init kpu input
data = np.zeros((1,3,320,320),dtype=np.uint8)
kpu_input = nn.from_numpy(data)
kpu.set_input_tensor(0, kpu_input)

print("inputs info:")
for i in range(kpu.inputs_size()):
    print(kpu.inputs_desc(i))

print("outputs info:")
for i in range(kpu.outputs_size()):
    print(kpu.outputs_desc(i))

# load input bin
data_file = "/sdcard/app/tests/nncase_runtime/face_detection/face_detection_ai2d_input.bin"
ai2d_input = np.fromfile(data_file, dtype=np.uint8)
ai2d_input = ai2d_input.reshape((1, 3, 624, 1024))
ai2d_input_tensor = nn.from_numpy(ai2d_input)


ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
               nn.ai2d_format.NCHW_FMT,
               np.uint8, np.uint8)
ai2d.set_pad_param(True, [0,0,0,0,0,125,0,0], 0, [104,117,123])
ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel )
ai2d_builder = ai2d.build([1,3,624,1024], [1,3,320,320])

# set ai2d output
ai2d_out = kpu.get_input_tensor(0)

# run
ai2d_builder.run(ai2d_input_tensor, ai2d_out)
kpu.run()

# get output
for i in range(kpu.outputs_size()):
    result = kpu.get_output_tensor(i)
    result = result.to_numpy()
    utime.sleep(1)
    #file_ = "/sdcard/app/output_{}.bin".format(i)
    #np.save(file_, result)
    print("result: ", i, result.flatten()[-5:])
    print(result.shape,result.dtype)

