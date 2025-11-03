# SEO Anahtar Kelime ve Rakip Analiz Araci

Bu klasor, `hairlines.blog` icin saglik turizmi odakli anahtar kelime ve rakip analizini hizli sekilde hazirlamak icin gelistirilmis Python tabanli bir komut satiri araci icerir.

## Ozellikler

- Google Trends (pytrends) uzerinden iliskili sorgular, bolgesel ilgi ve trend olan aramalar
- Google Suggest (otamatik tamamlama) sorgularindan benzersiz anahtar kelime onerileri
- DuckDuckGo arama sonucundan rakip domain/listeleri
- Ciktiyi Markdown veya JSON formatinda verebilme
- Coklu hedef bolge destegi (varsayilan: Turkiye, Almanya, Birlesik Krallik)

## Kurulum

Araci calistirmadan once bagimliliklari yukleyin:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r tools/seo_analyzer/requirements.txt
```

> Not: Sunucunuzda `python3-venv` paketi yoksa, alternatif olarak `python3 -m pip install --user -r tools/seo_analyzer/requirements.txt` komutunu calistirabilirsiniz.

## Kullanim

```bash
python tools/seo_analyzer/cli.py "hair transplant" --regions TR DE GB --timeframe "today 12-m" --format markdown
```

Baslica parametreler:

- `category`: Analiz edilecek cekirdek anahtar kelime/kategori.
- `--regions`: ISO ulke kodlari listesi (varsayilan `TR DE GB`).
- `--timeframe`: Google Trends zaman araligi (`now 7-d`, `today 3-m` gibi).
- `--format`: `markdown` veya `json`.
- `--output`: Ciktinin yazilacagi dosya yolu.

Ornek JSON cikti olusturma:

```bash
python tools/seo_analyzer/cli.py "hair transplant turkey" --format json --output reports/hairlines.json
```

## Notlar

- Google Trends ve DuckDuckGo servisleri zaman zaman oran sinirlamalarina tabi olabilir; hata durumunda komut ciktisinda uyarilar gorulebilir.
- DuckDuckGo sorgulari sadece bilgilendirme amaclidir; ticari kararlar icin ek dogrulama yapilmasi onerilir.
- Bolge listesinde yer almayan ulkeler icin arac varsayilan dil olarak `en-US` kullanir ve DuckDuckGo `wt-wt` (global) sonucunu getirir.
