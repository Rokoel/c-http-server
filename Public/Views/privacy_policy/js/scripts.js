window.addEventListener('scroll', function(e) {
    if(window.innerHeight - 50 <= window.scrollY){
        document.getElementById('hidden-header-bg').style.visibility = 'visible';
    }
    else{
        document.getElementById('hidden-header-bg').style.visibility = 'hidden';
    }
});

document.getElementById('scroller-1').addEventListener('click', function(e) {
    window.scrollTo({
        top: window.innerHeight - 40,
        behavior: "smooth"
    });
});