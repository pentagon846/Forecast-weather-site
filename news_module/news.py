import requests


# from newsapi import NewsApiClient


def get_news(news_api_key):
    url = f"https://newsapi.org/v2/top-headlines"
    params = {
        'country': 'us',
        'apiKey': news_api_key,
        'pageSize': 5
    }

    response = requests.get(url, params=params)
    # print(response)
    if response.status_code == 200:
        news_data = response.json()
        # print(news_data)
        return news_data['articles']
    return []
