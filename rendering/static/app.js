function refresh() {
    const img = new Image();
    img.src = "/rendering.png";
    img.style.display = "none";

    img.onload = () => {
        console.log("Load");
        $(img).show();
        refresh();
    };

    $('#app').prepend(img);
}

refresh();
