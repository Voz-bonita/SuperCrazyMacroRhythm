import os
from PIL import Image
import numpy as np
from tqdm import tqdm


def lesser(x, y):
    return x < y


def greater(x, y):
    return x > y


FPS = 60
WIDTH = 720
HEIGHT = 400

KEY_AMOUNT = 4
NOTE_HEIGHT = 20
NOTES_WIDTH = 30
NOTES_KEYS = ["f", "v", "Key.left", "Key.up"]
KEYS_TRIGGER = [
    ("blue", 160, greater),
    ("red", 90, greater),
    ("red", 50, lesser),
    ("red", 50, lesser),
]
notes_pressed = [False, False, False, False]
notes_last_frame = [-1, -1, -1, -1]
notes_last_key_stroke = [None, None, None, None]


song_dir = "songs/20-False_Data"
images = sorted(
    os.listdir(f"{song_dir}/images"),
    key=lambda x: int(x.replace("out", "").replace(".png", "")),
)
key_strokes = []

for frame, image in tqdm(enumerate(images)):
    timming = (frame + 1) / 60
    img_obj = Image.open(f"{song_dir}/images/{image}")
    img_array = np.array(img_obj)
    for i in range(KEY_AMOUNT):
        width = NOTES_WIDTH * i + 15
        key = NOTES_KEYS[i]
        pressed = notes_pressed[i]
        color, threshold, compare_func = KEYS_TRIGGER[i]
        is_slider = frame - notes_last_frame[i] >= 15

        note_box = img_array[: NOTE_HEIGHT // 2, width - 3 : width + 3, :]

        rgb_mean = {
            "red": note_box[:, :, 0].mean(),
            "green": note_box[:, :, 1].mean(),
            "blue": note_box[:, :, 2].mean(),
        }
        note_triggered = compare_func(rgb_mean[color], threshold)

        if note_triggered and not pressed:
            notes_pressed[i] = True
            notes_last_frame[i] = frame
            key_strokes.append([key, "1", f"{timming + 1 / 60:.3f}"])
            notes_last_key_stroke[i] = len(key_strokes) - 1
        elif not note_triggered and pressed:
            notes_pressed[i] = False
            if not is_slider:
                # timming = (notes_last_frame[i] + 2) / 60
                key_strokes[notes_last_key_stroke[i]][1] = "2"
            else:
                key_strokes.append((key, "0", f"{timming + 1 / 60:.3f}"))
                notes_last_key_stroke[i] = len(key_strokes) - 1

key_strokes = sorted(key_strokes, key=lambda x: x[2])

with open(f"{song_dir}/keystrokes.txt", "w+") as file:
    for sequence in key_strokes:
        file.write(" ".join(sequence))
        file.write("\n")
