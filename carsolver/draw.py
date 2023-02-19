from PIL import Image, ImageDraw
import json


COLOR_BACKGROUND = (128, 128, 128)
COLOR_BORDER_CAR = (255, 255, 255)
COLOR_BORDER_FIELD = (182, 195, 179)
COLOR_HERO = (200,66,77)
COLOR_VILLAIN_2 = (155, 155, 155)
COLOR_VILLAIN_3_DOWN = (0, 155, 0)
COLOR_VILLAIN_3_RIGHT = (155, 155, 0)
COLOR_VILLAIN_DEFALUT = (55, 44, 192)
MARGIN_CAR = 0.1
MARGIN_FIELD = 0.5
SIZE_CELL = 100


def abstractRectangleToCoordinates(x, y, xsize, ysize):
    return addFieldMargins(x + MARGIN_CAR, y + MARGIN_CAR, x + xsize - MARGIN_CAR, y + ysize - MARGIN_CAR)


def addFieldMargins(left, top, right, bottom):
    return tuple((i + MARGIN_FIELD)*SIZE_CELL for i in (left, top, right, bottom))


def drawCar(ctx, x, y, xsize, ysize, color):
    ctx.rectangle(abstractRectangleToCoordinates(x, y, xsize, ysize), fill=color, outline=COLOR_BORDER_CAR)


def sizeAndDirToColor(size, direction):
    if size == 2:
        return COLOR_VILLAIN_2
    elif size == 3 and direction == 'down':
        return COLOR_VILLAIN_3_DOWN
    elif size == 3 and direction == 'right':
        return COLOR_VILLAIN_3_RIGHT
    return COLOR_VILLAIN_DEFALUT


def jsonToAbstractRectangle(car):
    size, direction = car['size'], car['dir']
    if direction in ("up","down"):
        xsize = 1
        ysize = size
    elif direction in ("left","right"):
        xsize = size
        ysize = 1
    else:
        print("Invalid direction")
        exit(0)
    carx, cary = car['x'], car['y']
    if direction in ("up","left"):
        x = (1 + carx - xsize)
        y = (1 + cary - ysize)
    else:
        x = carx
        y = cary
    return x, y, xsize, ysize


with open("solution.json") as s:
    data = json.loads(s.read())


for i, stage in enumerate(data['solution']):
    im = Image.new('RGB', tuple(round((data['field'][i] + 2*MARGIN_FIELD)*SIZE_CELL) for i in ('width', 'height')), COLOR_BACKGROUND)
    ctx = ImageDraw.Draw(im)
    ctx.rectangle(addFieldMargins(0, 0, data['field']['width'], data['field']['height']), fill=COLOR_BACKGROUND, outline=COLOR_BORDER_FIELD)
    for villain in stage['cars'][1:]:
        drawCar(ctx, *jsonToAbstractRectangle(villain), sizeAndDirToColor(villain['size'], villain['dir']))
    drawCar(ctx, *jsonToAbstractRectangle(stage['cars'][0]), COLOR_HERO)
    im.save('solution' + str(i) + '.jpg', quality=95)

