/**
 * Created by Orthocenter on 1/2/15.
 */

tiles_l = 256;
real_tiles_l = 0;
tiles_filepath = "Tiles/";

function getMouseCoordFromEvent(event) {
    var x = 0;
    var y = 0;

    var element = event.target;
    do {
        x += element.offsetLeft;
        y += element.offsetTop;
    } while (element = element.offsetParent);

    x = (window.pageXOffset + event.clientX) - x;
    y = (window.pageYOffset + event.clientY) - y;

    return {x: x, y: y};
}

function onCanvasMouseEvent(event) {
    event.preventDefault();
    var coord = getMouseCoordFromEvent(event);
    if (event.type == "mousedown") {
        dragging = true;
        lastCoord = coord;
    }
    else if (event.type == "mousemove") {
        if (dragging) {
            updateTLPixel(-(coord.x - lastCoord.x), -(coord.y - lastCoord.y));
            lastCoord = coord;
            drawTiles();
        }
    }
    else if (event.type == "mouseup") {
        dragging = false;
    }
    else if (event.type == "mousewheel") {
        if (typeof wheelDelta == "undefined")
            wheelDelta = 0;
        else
            wheelDelta += event.wheelDelta;

        if (wheelDelta < -100) {
            wheelDelta = 0;
            if (level - 1 >= minLevel) {
                level--;
                setTLPixel(tl_px / 2 - coord.x / 2, tl_py / 2 - coord.y / 2);
                drawTiles();
            }
        }

        if (wheelDelta > 100) {
            wheelDelta = 0;
            if (level + 1 <= maxLevel) {
                level++;
                setTLPixel(tl_px * 2 + coord.x, tl_py * 2 + coord.y);
                drawTiles();
            }
        }
    }
    else if (event.type == "dblclick") {
        console.log(event)
        if (!event.altKey) {
            if (level + 1 <= maxLevel) {
                level++;
                setTLPixel(tl_px * 2 + coord.x, tl_py * 2 + coord.y);
                drawTiles();
            }
        }
        else
        {
            if (level - 1 >= minLevel) {
                level--;
                setTLPixel(tl_px / 2 - coord.x / 2, tl_py / 2 - coord.y / 2);
                drawTiles();
            }
        }
    }
}

function getTouchCoordFromEvent(event) {
    return {x: event.targetTouches[0].pageX,
        y: event.targetTouches[0].pageY};
}

function onCanvasTouchEvent(event) {
    event.preventDefault();
    var coord = getTouchCoordFromEvent(event);
    if (event.type == "touchstart") {
        dragging = true;
        lastCoord = coord;
    }
    else if (event.type == "touchmove" && event.targetTouches.length == 1) {
        if (dragging) {
            updateTLPixel(-(coord.x - lastCoord.x), -(coord.y - lastCoord.y));
            lastCoord = coord;
            drawTiles();
        }
    }
    else if (event.type == "touchmove" && event.targetTouches.length == 2) {
        if (typeof lastTouchesDist == "undefined") lastTouchesDist = 0;

        if (typeof touchesDelta == "undefined") {
            touchesDelta = 0;
        } else {
            var dist = Math.abs(event.targetTouches[0].pageX - event.targetTouches[1].pageX) +
                Math.abs(event.targetTouches[0].pageY - event.targetTouches[1].pageY);
            touchesDelta += dist - lastTouchesDist;
            lastTouchesDist = dist;
        }

        var centerX = (event.targetTouches[0].pageX + event.targetTouches[1].pageX) / 2;
        var centerY = (event.targetTouches[0].pageY + event.targetTouches[1].pageY) / 2;

        if (touchesDelta < -30) {
            touchesDelta = 0;
            if (level - 1 >= minLevel) {
                level--;
                setTLPixel(tl_px / 2 - centerX / 2, tl_py / 2 - centerY / 2);
                drawTiles();
            }
        }

        if (touchesDelta > 30) {
            touchesDelta = 0;
            if (level + 1 <= maxLevel) {
                level++;
                setTLPixel(tl_px * 2 + centerX, tl_py * 2 + centerY);
                drawTiles();
            }
        }
    }
    else if (event.type == "touchend") {
        dragging = false;
    }
}

function WGS84_to_px_py(lat, lon, level) {
    var sinLat = Math.sin(lat * Math.PI / 180);
    var n = Math.pow(2, level);

    var px = (lon + 180.0) / 360.0 * tiles_l * n;
    var py = (0.5 - Math.log((1 + sinLat) / (1 - sinLat)) / (4 * Math.PI)) * tiles_l * n;

    return {px: px, py: py}
}

function WGS84_to_tx_ty(lat, lon, level) {
    var sinLat = Math.sin(lat * Math.PI / 180);
    var n = Math.pow(2, level);
    var tx = Math.floor((lon + 180) / 360 * n);
    var ty = Math.floor(0.5 - log((1 + sinLat) / (1 - sinLat)) / (4 * Math.PI)) * n;

    return {tx: tx, ty: ty};
}

function setTLTxTy(tx, ty) {
    tl_tx = Math.floor(tx);
    tl_ty = Math.floor(ty);

    tl_px = tx * tiles_l;
    tl_py = ty * tiles_l;

    tl_ox = 0;
    tl_oy = 0;
}

function setTLPixel(px, py) {
    tl_px = px;
    tl_py = py;

    tl_tx = Math.floor(tl_px / tiles_l);
    tl_ty = Math.floor(tl_py / tiles_l);

    tl_ox = Math.floor(tl_tx * tiles_l - px) * retina_ratio;
    tl_oy = Math.floor(tl_ty * tiles_l - py) * retina_ratio;
}

function updateTLPixel(dpx, dpy) {
    setTLPixel(tl_px + dpx, tl_py + dpy);
}

function setCanvasSize() {
    devicePixelRatio = window.devicePixelRatio || 1;
    backingStoreRatio = context.webkitBackingStorePixelRatio || 1;
    retina_ratio = (devicePixelRatio / backingStoreRatio) > 1 ? 2 : 1;
    real_tiles_l = tiles_l * retina_ratio;
    canvas.width = window.innerWidth * retina_ratio;
    canvas.height = window.innerHeight * retina_ratio;
    canvas.style.height = window.innerHeight + "px";
    canvas.style.width = window.innerWidth + "px";
}

function setDrawParam() {
    numTilesX = (canvas.width / real_tiles_l) + 1;
    numTilesY = (canvas.height / real_tiles_l) + 1;

    numTiles = numTilesX * numTilesY;

    maxLevel = 17;
    minLevel = 14;
}

function drawTiles() {
    var tx_begin = tl_tx;
    var tx_end = tl_tx + numTilesX;
    var ty_begin = tl_ty;
    var ty_end = tl_ty + numTilesY;

    for (var tx = tx_begin; tx < tx_end; tx++) {
        for (var ty = ty_begin; ty < ty_end; ty++) {
            if (retina_ratio == 1) {
                var tile_filename = tiles_filepath + tx + "_" + ty + "_" + level + ".png";
            } else {
                var tile_filename = tiles_filepath + tx + "_" + ty + "_" + level + "_r.png";
            }

            context.fillStyle = "#f7f5f0";
            context.fillRect(real_tiles_l * (tx - tl_tx) + tl_ox, real_tiles_l * (ty - tl_ty) + tl_oy
                , real_tiles_l, real_tiles_l);

            var tile = new Image();
            tile.src = tile_filename;
            tile.tx = tx;
            tile.ty = ty;
            tile.onload = function () {
                drawTile(this);
            }
        }
    }

    function drawTile(tile) {
        context.drawImage(tile, real_tiles_l * (tile.tx - tl_tx) + tl_ox, real_tiles_l * (tile.ty - tl_ty) + tl_oy);
    }
}

function onload() {
    canvas = document.getElementById("main");
    context = canvas.getContext("2d");

    dragging = false;
    canvas.addEventListener("mousedown", onCanvasMouseEvent);
    canvas.addEventListener("mouseup", onCanvasMouseEvent);
    canvas.addEventListener("mousemove", onCanvasMouseEvent);
    canvas.addEventListener("mousewheel", onCanvasMouseEvent);
    canvas.addEventListener("dblclick", onCanvasMouseEvent);

    canvas.addEventListener("touchstart", onCanvasTouchEvent);
    canvas.addEventListener("touchmove", onCanvasTouchEvent);
    canvas.addEventListener("touchend", onCanvasTouchEvent);

    setCanvasSize();
    setDrawParam();
    level = 17;
    n = Math.pow(2, level);

    var tl_tx = 109772;
    var tl_ty = 53548;

    console.log(context);

    setTLTxTy(tl_tx, tl_ty);
    console.log(tl_px, tl_py);

    drawTiles();
}

window.addEventListener('load', onload, false);