import urllib.request

BASE_URL = "https://banjo-storage.s3.amazonaws.com"

def get_url(path):
    return f"{BASE_URL}/{path}"

def request(path):
    url = get_url(path)
    return urllib.request.urlopen(url)

def download(path, destination):
    url = get_url(path)
    urllib.request.urlretrieve(url, destination)
