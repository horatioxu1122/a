#!/usr/bin/env python3
"""Grab full paper text from Google Scholar profile via Playwright. Downloads PDFs, extracts text."""
import subprocess, sys, os, time, tempfile

VENV = os.path.join(os.path.dirname(os.path.abspath(__file__)), '.scholar_venv')
PY = os.path.join(VENV, 'bin', 'python3')
try:
    from playwright.sync_api import sync_playwright
    import pdfplumber
except ImportError:
    if not os.path.exists(VENV):
        subprocess.run([sys.executable, '-m', 'venv', VENV], check=True)
    subprocess.run([PY, '-m', 'pip', 'install', '-q', 'playwright', 'pdfplumber'], check=True)
    subprocess.run([PY, '-m', 'playwright', 'install', 'chromium'], check=True)
    os.execv(PY, [PY] + sys.argv)

from playwright.sync_api import sync_playwright
import pdfplumber

URL = sys.argv[1] if len(sys.argv) > 1 else "https://scholar.google.com/citations?user=0epc43IAAAAJ&hl=en"
N = int(sys.argv[2]) if len(sys.argv) > 2 else 5
OUTDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'scholar_papers')
DLDIR = tempfile.mkdtemp(prefix='scholar_dl_')
os.makedirs(OUTDIR, exist_ok=True)

def safe_filename(s):
    return ''.join(c if c.isalnum() or c in ' -_' else '_' for c in s)[:80].strip()

def pdf_to_text(path):
    try:
        with pdfplumber.open(path) as pdf:
            return '\n\n'.join(pg.extract_text() or '' for pg in pdf.pages)
    except Exception as e:
        return f'[PDF extraction failed: {e}]'

def try_download_pdf(ctx, url):
    """Download PDF via direct HTTP since Playwright blocks PDF navigations."""
    import urllib.request, ssl
    sctx = ssl.create_default_context()
    sctx.check_hostname = False
    sctx.verify_mode = ssl.CERT_NONE
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36'})
        resp = urllib.request.urlopen(req, timeout=30, context=sctx)
        ct = resp.headers.get('Content-Type', '')
        data = resp.read()
        if b'%PDF' in data[:20] or 'pdf' in ct.lower():
            path = os.path.join(DLDIR, f'paper_{time.time():.0f}.pdf')
            with open(path, 'wb') as f: f.write(data)
            return path
    except Exception as e:
        print(f"    Download error: {e}")
    return None

def grab_paper_text(ctx, detail_url, title, idx):
    page = ctx.new_page()
    try:
        page.goto(detail_url, wait_until='networkidle', timeout=20000)
        time.sleep(1)
        detail_text = page.inner_text('body')
        # find source links
        links = page.eval_on_selector_all('#gsc_oci_title_gg a, .gsc_oci_title_ggi a',
            'els => els.map(e => ({href: e.href, text: e.innerText}))')
        source_text = ''
        source_url = ''
        for link in links:
            href = link.get('href', '')
            if href and 'scholar.google' not in href and 'javascript:' not in href:
                source_url = href
                break
        if not source_url:
            return detail_text, '', ''
        print(f"  [{idx}] Source: {source_url[:90]}")
        is_pdf = source_url.lower().endswith('.pdf') or '/pdf/' in source_url.lower()
        if is_pdf:
            pdf_path = try_download_pdf(ctx, source_url)
            if pdf_path:
                source_text = pdf_to_text(pdf_path)
                print(f"  [{idx}] PDF extracted: {len(source_text)} chars")
            else:
                print(f"  [{idx}] PDF download failed")
        else:
            # HTML source - try to get page text
            try:
                page.goto(source_url, wait_until='domcontentloaded', timeout=20000)
                time.sleep(2)
                source_text = page.inner_text('body')
            except Exception as e:
                # might be a PDF that didn't have .pdf extension
                pdf_path = try_download_pdf(ctx, source_url)
                if pdf_path:
                    source_text = pdf_to_text(pdf_path)
                    print(f"  [{idx}] PDF (redirect) extracted: {len(source_text)} chars")
                else:
                    print(f"  [{idx}] Source failed: {e}")
        return detail_text, source_text, source_url
    except Exception as e:
        print(f"  [{idx}] Detail failed: {e}")
        return '', '', ''
    finally:
        page.close()

with sync_playwright() as p:
    browser = p.chromium.launch(headless=True, args=['--disable-blink-features=AutomationControlled'])
    ctx = browser.new_context(accept_downloads=True, user_agent='Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36')
    page = ctx.new_page()
    page.goto(URL, wait_until='networkidle', timeout=30000)
    rows = page.locator('#gsc_a_b .gsc_a_tr')
    count = min(rows.count(), N)
    papers = []
    for i in range(count):
        link = rows.nth(i).locator('.gsc_a_t a')
        title = link.inner_text() if link.count() else f'paper_{i}'
        href = link.get_attribute('href') if link.count() else ''
        if href and not href.startswith('http'):
            href = 'https://scholar.google.com' + href
        papers.append((title, href))
    page.close()
    for i, (title, href) in enumerate(papers):
        print(f"\n[{i+1}/{count}] {title}")
        detail_text, source_text, source_url = grab_paper_text(ctx, href, title, i+1)
        fname = safe_filename(title)
        out = os.path.join(OUTDIR, f'{i+1:02d}_{fname}.txt')
        with open(out, 'w') as f:
            f.write(f"TITLE: {title}\nSOURCE: {source_url}\n{'='*80}\n\n")
            f.write("--- SCHOLAR DETAIL ---\n")
            f.write(detail_text)
            if source_text:
                f.write(f"\n\n{'='*80}\n--- FULL PAPER TEXT ---\n")
                f.write(source_text)
            else:
                f.write("\n\n[No full text obtained]\n")
        sz = os.path.getsize(out)
        print(f"  -> {sz} bytes: {out}")
    browser.close()
print(f"\nDone. {count} papers in {OUTDIR}/")
