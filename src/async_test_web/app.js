function loadData() {
    document.getElementById("test").innerHTML = "Please wait...";
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "data/monkeys", true);
    xhttp.onprogress = function () {
        document.getElementById("test").innerHTML = "<i>" + xhttp.responseText + "</i>";
    };
    xhttp.onreadystatechange = function () {
        if (xhttp.readyState == 4 && xhttp.status == 200) {
            document.getElementById("test").innerHTML = xhttp.responseText;
        }
    };
    xhttp.send();
}
