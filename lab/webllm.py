#!/usr/bin/env python3
"""webllm — test LLM web platforms via raw CDP. Detect sign-in, query once.
Usage: python3 webllm.py [me] [canary]
"""
import sys,os,time,json
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
import agui

SITES={# name: (url, input_sel, signin_texts, response_js)
# response_js: JS expression returning last assistant response text
'claude':    ('https://claude.ai/',      'div[contenteditable="true"]', ['Sign in','Log in','Sign up'],
    '(()=>{let a=document.querySelectorAll("[data-testid*=message],[class*=message-content],[class*=response]");for(let i=a.length-1;i>=0;i--){let t=a[i].innerText?.trim();if(t&&t.length>2)return t}return""})()'),
'chatgpt':   ('https://chatgpt.com/',    'div[contenteditable="true"],#prompt-textarea', ['Log in','Sign up','Sign in'],
    '[...document.querySelectorAll("div[data-message-author-role=assistant] .markdown")].pop()?.innerText||""'),
'grok':      ('https://grok.com/',       'div[contenteditable="true"],textarea', ['Sign in','Log in','Sign up'],
    '[...document.querySelectorAll(".message-text,.response")].pop()?.innerText||""'),
'deepseek':  ('https://chat.deepseek.com/','textarea', ['Sign In','Log In','Sign Up'],
    '[...document.querySelectorAll(".ds-markdown")].pop()?.innerText||""'),
'gemini':    ('https://gemini.google.com/app','div[role="textbox"],textarea', ['Sign in','Sign up'],
    '[...document.querySelectorAll("message-content .markdown")].pop()?.innerText||""'),
}

def _js(expr):
    """Eval JS on current page, return value"""
    return agui.get_page()._eval(expr)

def check_signin(name):
    """Check if signed in by looking for sign-in buttons. Returns (signed_in:bool, detail:str)"""
    url, input_sel, signin_texts, _ = SITES[name]

    # Check for sign-in/login buttons in visible text
    for txt in signin_texts:
        found = _js(f'''(()=>{{
            let els = document.querySelectorAll('a,button,span,div');
            for(let e of els) {{
                let t = e.textContent?.trim();
                if(t === {json.dumps(txt)} && e.offsetParent !== null) return t;
            }}
            return null;
        }})()''')
        if found:
            return False, f'found "{found}" button'

    # Check for input field (means we're logged in and on the chat page)
    has_input = _js(f'!!document.querySelector({json.dumps(input_sel)})')
    if has_input:
        return True, 'input field present'

    return None, 'unclear — no sign-in button, no input field'

def test_site(name, query='What is 2+2? Answer in one word.'):
    """Navigate to site, check sign-in, optionally send query"""
    url, input_sel, _, resp_js = SITES[name]
    page = agui.get_page()

    print(f'\n{"="*50}')
    print(f'  {name.upper()}: {url}')
    print(f'{"="*50}')

    # Navigate
    page.goto(url, timeout=20000)
    time.sleep(4)

    final_url = _js('location.href')
    print(f'  URL: {final_url}')

    # CDP info
    title = _js('document.title')
    cookies = page._send('Network.getCookies', {'urls': [url]})
    nc = len(cookies.get('cookies', []))
    print(f'  Title: {title}')
    print(f'  Cookies: {nc}')

    # Sign-in check
    signed_in, detail = check_signin(name)
    status = '✓ SIGNED IN' if signed_in else ('✗ NOT SIGNED IN' if signed_in == False else '? UNKNOWN')
    print(f'  Auth: {status} ({detail})')

    # Screenshot
    shot = f'/tmp/webllm_{name}.png'
    page.screenshot(path=shot)
    print(f'  Shot: {shot}')

    if not signed_in:
        return False

    # Try sending a query
    print(f'  Query: {query}')
    sels = input_sel.split(',')
    sent = False
    for sel in sels:
        has = _js(f'!!document.querySelector({json.dumps(sel)})')
        if has:
            page.fill(sel, query)
            time.sleep(0.3)
            page.keyboard.press('Enter')
            sent = True
            print(f'  Sent via: {sel}')
            break

    if not sent:
        print(f'  ✗ No input field found')
        return True

    # Event-driven response — MutationObserver on body, diff against baseline
    baseline = _js('document.body.innerText') or ''
    print(f'  ', end='', flush=True)
    total = 0
    for chunk, full in page.watch_text(timeout=60):
        # Only show text beyond baseline + query echo
        resp = full[len(baseline):] if len(full) > len(baseline) else ''
        resp = resp.replace(query, '', 1).strip()
        if len(resp) > total:
            new = resp[total:]
            # Filter UI chrome
            lines = [l for l in new.split('\n') if l.strip()
                     and not any(x in l for x in ['is AI and can','double-check','Please verify'])]
            out = '\n'.join(lines)
            if out:
                print(out, end='', flush=True)
            total = len(resp)
    print()
    if total: print(f'  [{total} chars]')
    else: print('  (no response captured)')

    page.screenshot(path=f'/tmp/webllm_{name}_resp.png')
    print(f'  Shot: /tmp/webllm_{name}_resp.png')
    return True

if __name__ == '__main__':
    args = set(sys.argv[1:])
    if 'me' in args: agui._use_real_profile = True; args.discard('me')
    if 'canary' in args: agui._chrome_variant = 'canary'; args.discard('canary')

    # Specific sites or all
    sites = [s for s in args if s in SITES] or list(SITES.keys())

    print(f'CDP mode | profile={"user" if agui._use_real_profile else "isolated"} | variant={agui._chrome_variant}')
    agui.launch_browser_with_positioning()
    page = agui.get_page()

    # CDP connection info
    print(f'Page type: {type(page).__name__}')
    ua = _js('navigator.userAgent')
    print(f'UA: {ua[:80]}...')

    results = {}
    for name in sites:
        try:
            ok = test_site(name)
            results[name] = ok
        except Exception as e:
            print(f'  ✗ ERROR: {e}')
            results[name] = None

    print(f'\n{"="*50}')
    print('  RESULTS:')
    for name, ok in results.items():
        s = '✓' if ok else ('✗' if ok == False else '?')
        print(f'    {s} {name}')
    print(f'{"="*50}')

    agui.close_browser()
