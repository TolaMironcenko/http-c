const body = document.getElementsByTagName('body')
alert("hello world");

fetch("http://127.0.0.1:8080/example").then(data => data.json()).then(jsondata => console.log(jsondata))
