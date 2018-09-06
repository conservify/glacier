const cache = {};

let previous = null;

function getQueryVariable(variable, defaultValue) {
    var query = window.location.search.substring(1);
    var vars = query.split('&');
    for (var i = 0; i < vars.length; i++) {
        var pair = vars[i].split('=');
        if (decodeURIComponent(pair[0]) == variable) {
            return decodeURIComponent(pair[1]);
        }
    }
    return defaultValue;
}

function hourChanged(previous, current) {
    return previous.Hour != current.Hour || previous.NumberOfFiles != current.NumberOfFiles;
}

function shouldRender(data) {
    const rendering = [];

    if (data.PreviousHour == null || data.CurrentHour == null) {
        return rendering;
    }

    if (previous == null) {
        if (data.AvailableHours.length > 2) {
            rendering.push(data.AvailableHours[data.AvailableHours.length - 3]);
        }
        if (data.PreviousHour) {
            rendering.push(data.PreviousHour.Hour);
        }
        rendering.push(data.CurrentHour.Hour);
    }
    else {
        if (hourChanged(previous.PreviousHour, data.PreviousHour)) {
            console.log("Previous Hour Changed", previous.PreviousHour, data.PreviousHour);
            rendering.push(data.PreviousHour.Hour);
        }
        if (hourChanged(previous.CurrentHour, data.CurrentHour)) {
            console.log("Current Hour Changed", previous.CurrentHour, data.CurrentHour);
            rendering.push(data.CurrentHour.Hour);
        }
    }

    previous = data;

    console.log("Will render", rendering);

    return rendering;
}

function refresh() {
    $.getJSON('/status.json', (data) => {
        const r = shouldRender(data);
        if (r.length == 0) {
            setTimeout(() => {
                refresh();
            }, 30 * 1000);
        }
        else {
            for (let i = 0; i < r.length; i++) {
                const hour = r[i];
                const existing = cache[hour];

                console.log(hour, "Loading", existing);

                const img = cache[hour] = new Image();
                img.src = "/rendering.png?hour=" + hour + "&axis=" + getQueryVariable("axis", "x");
                img.style.display = "none";

                img.onload = () => {
                    if (existing) {
                        $(existing).replaceWith(img);
                        console.log(hour, "Done, replaced");
                    }
                    else {
                        console.log(hour, "Done, first");
                    }

                    $(img).show();

                    if (i == r.length - 1) {
                        refresh();
                    }
                };

                $('#app').prepend(img);
            }
        }
    });
}

refresh();
