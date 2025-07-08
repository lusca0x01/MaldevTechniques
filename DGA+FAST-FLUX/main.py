from time import time, sleep, gmtime, strftime
from sys import argv
from random import randint
from dnslib import RR, A
from dnslib.server import DNSServer
from random import choice
from threading import Thread, Event
import logging

logging.basicConfig(level=logging.INFO)


TLD = ["com", "net", "org", "info", "biz", "edu", "gov", "mil", "int",
       "app", "blog", "shop", "tech", "online", "store", "website", "site",
       "company", "cloud", "xyz", "design", "media", "news", "pro", "social",
       "space", "tips", "world", "co", "agency", "consulting", "financial", "law"]

COUNTRY_TLD = ["us", "ca", "mx", "br", "ar", "cl", "co", "pe", "ve",
               "uk", "de", "fr", "it", "es", "nl", "se", "no", "dk", "fi",
               "pl", "ru", "cn", "jp", "in", "au", "nz", "za", "eg", "sa",
               "ae", "kr", "sg", "hk", "tw", "my", "ph", "th", "id", "vn",
               "pk", "bd", "np", "lk"]

ip_pool = [f"192.168.4.{x}" for x in range(1, 254)]

domain_in_use = ""

stop_event = Event()

class FastFluxResolver:
    def resolve(self, request, handler):
        global domain_in_use

        domain = str(request.q.qname)
        qtype = request.q.qtype
        reply = request.reply()

        logging.info(f"Received request for {domain} (type {qtype}), (actual domain: {domain_in_use})")

        if domain == f"{domain_in_use}." and qtype == 1:
            selected_ip = choice(ip_pool)
            logging.info(f"Resolving {domain} to {selected_ip}")
            reply.add_answer(RR(domain, qtype, rdata=A(selected_ip), ttl=300))
        
        return reply

def lcg(n):
    # LCG https://en.wikipedia.org/wiki/Linear_congruential_generator
    # Mersenne num
    return (82589933 * n + 1337) % 2**32


def redc(n):
    return (n // 256)


def gen_domain(seed, time_s):
    logging.info(f"Last time used: {strftime("%D %T", gmtime(time_s))}")
    time_seed = int(time_s)
    rand = seed + time_seed
    domain_l = randint(12, 16)

    domain = ""

    for _ in range(domain_l):
        rand = redc(lcg(rand))
        ch = rand % 26 + 97
        domain += chr(ch)

    return domain + "." + TLD[redc(lcg(rand)) % len(TLD)] + "." + COUNTRY_TLD[redc(lcg(rand)) % len(COUNTRY_TLD)]

def retrieve_domains(seed):
    global domain_in_use 
    while not stop_event.is_set():
        domains = [gen_domain(seed=seed, time_s=time() + i * 3600) for i in range(24)]
        while len(domains) > 0:
            domain_in_use = choice(domains)
            logging.info(f"Selected domain: {domain_in_use}")
            if stop_event.is_set():
                break
            sleep(30)


if __name__ == "__main__":
    seed = 12345
    
    try:
        seed = int(argv[1])
    except (IndexError, ValueError):
        pass

    dga_thr = Thread(target=retrieve_domains, args=(seed,), name="DGA")
    dga_thr.start()

    resolver = FastFluxResolver()
    dns_server = DNSServer(resolver, port=8053, address="0.0.0.0")
    dns_server.start_thread()

    try:
        while True:
            sleep(1)
    except KeyboardInterrupt:
        logging.info("Shutting down...")
        dns_server.stop()
        stop_event.set()
        dga_thr.join(timeout=5)

        if dga_thr.is_alive():
            logging.error("DGA thread did not stop in time, forcing exit.")
            quit()
        
        logging.info("DNS Server stopped.")