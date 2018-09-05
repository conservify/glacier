let previous = null;

function shouldRender(data) {
    if (previous == null || previous.NumberOfFiles != data.NumberOfFiles || previous.Start != data.Start || previous.End != data.End) {
        previous = data;
        return true;
    }
    return false;
}

function refresh() {

    $.getJSON('/status.json', (data) => {
        if (shouldRender(data)) {
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
        else {
            setTimeout(() => {
                refresh();
            }, 10 * 1000);
        }
    });
}

refresh();
