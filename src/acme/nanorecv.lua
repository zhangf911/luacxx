acme = acme or {};

local function printf(msg, ...)
    print(string.format(msg, ...));
end;

function acme.nanorecv(url)
    require "nanomsg";

    local sock = nn_socket(AF_SP, NN_PULL);
    assert(sock >= 0);

    if nn_bind(sock, url) < 0 then
        print(nn_strerror(errno()));
    end;

    while true do
        local bytes, buf = nn_recv(sock, 0, NN_MSG, 0);
        assert(bytes >= 0);
        printf("recv: '%s'", buf);
        nn_freemsg (buf);
    end;
end;
