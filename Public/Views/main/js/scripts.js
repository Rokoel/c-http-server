window.addEventListener('scroll', function(e) {
    if(window.innerHeight - 50 <= window.scrollY){
        document.getElementById('header').style.backgroundColor = '#000000';
    }
    else{
        document.getElementById('header').style.backgroundColor = 'transparent';
    }
});

document.getElementById('scroller-1').addEventListener('click', function(e) {
    window.scrollTo({
        top: window.innerHeight - 40,
        behavior: "smooth"
    });
});